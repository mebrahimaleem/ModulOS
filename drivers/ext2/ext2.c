/* ext2.c - Second Extended File System driver implementation */
/* Copyright (C) 2026  Ebrahim Aleem
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <https://www.gnu.org/licenses/>
*/

#include <stdint.h>

#include <ext2/ext2.h>

#include <disk/disk.h>

#include <kernel/core/alloc.h>
#include <kernel/core/logging.h>
#include <kernel/core/fs.h>
#include <kernel/core/lock.h>
#include <kernel/core/panic.h>
#include <kernel/core/process.h>
#include <kernel/core/scheduler.h>
#include <kernel/core/kentry.h>

#include <kernel/lib/kmemcmp.h>
#include <kernel/lib/kmemcpy.h>
#include <kernel/lib/kmemset.h>
#include <kernel/lib/kstrlen.h>
#include <kernel/lib/kstrcmp.h>

#define SUPERBLOCK_LBA			2
#define SUPERBLOCK_SECTORS	2

#define DIRECT_BLOCKS			12
#define INDIR_1						12
#define INDIR_2						13
#define INDIR_3						14

#define EXT2_SUPER_MAGIC	0xEF53

#define EXT2_ROOT_INO			2

#define EXT2_S_IFREG	0x8000
#define EXT2_S_IFDIR	0x4000

#define EXT2_FT_REG_FILE	1
#define EXT2_FT_DIR				2

#define MODE_DIR			0x1

struct ext2_superblock_t {
	uint32_t s_inodes_count;
	uint32_t s_blocks_count;
	uint32_t s_r_blocks_count;
	uint32_t s_free_blocks_count;
	uint32_t s_free_inodes_count;
	uint32_t s_first_data_block;
	uint32_t s_log_block_size;
	uint32_t s_log_frag_size;
	uint32_t s_blocks_per_group;
	uint32_t s_frags_per_group;
	uint32_t s_inodes_per_group;
	uint32_t s_mtime;
	uint32_t s_wtime;
	uint16_t s_mnt_count;
	uint16_t s_max_mnt_count;
	uint16_t s_magic;
	uint16_t s_state;
	uint16_t s_errors;
	uint16_t s_minor_rev_level;
	uint32_t s_lastcheck;
	uint32_t s_checkinterval;
	uint32_t s_creator_os;
	uint32_t s_rev_level;
	uint16_t s_def_resuid;
	uint16_t s_def_resgid;

	/* ext2_dynamic_rev */
	uint32_t s_first_ino;
	uint16_t s_inode_size;
	uint16_t s_block_group_nr;
	uint32_t s_feature_compat;
	uint32_t s_feature_incompat;
	uint32_t s_feature_ro_compat;
	uint8_t s_uuid[16];
	uint8_t s_volume_name[16];
	uint8_t s_last_mounted[64];
	uint32_t s_algo_bitmap;

	/* perf. hints */
	uint8_t s_prealloc_blocks;
	uint8_t s_prealloc_dir_blocks;
	uint16_t align;

	/* journaling */
	uint8_t s_journal_uuid[16];
	uint32_t s_hournal_inum;
	uint32_t s_journal_dev;
	uint32_t s_last_orphan;

	/* dir. indexing */
	uint32_t s_hash_seed[4];
	uint8_t s_def_hash_version;
	uint8_t resv0[3];

	/* other */
	uint32_t s_default_mount_options;
	uint32_t s_first_meta_bg;
	uint8_t resv1[760];
} __attribute__((packed));

struct ext2_bg_desc_t {
	uint32_t bg_block_bitmap;
	uint32_t bg_inode_bitmap;
	uint32_t bg_inode_table;
	uint16_t bg_free_blocks_count;
	uint16_t bg_free_inodes_count;
	uint16_t bg_used_dirs_count;
	uint16_t bg_pad;
	uint8_t bg_reserved[12];
} __attribute__((packed));

struct ext2_inode_t {
	uint16_t i_mode;
	uint16_t i_uid;
	uint32_t i_size;
	uint32_t i_atime;
	uint32_t i_ctime;
	uint32_t i_mtime;
	uint32_t i_dtime;
	uint16_t i_gid;
	uint16_t i_links_count;
	uint32_t i_blocks;
	uint32_t i_flags;
	uint32_t i_osd1;
	uint32_t i_block[15];
	uint32_t i_generation;
	uint32_t i_file_acl;
	uint32_t i_dir_acl;
	uint32_t i_faddr;
	uint8_t i_osd2[12];
} __attribute__((packed));

struct ext2_ll_dir_entry_t {
	uint32_t inode;
	uint16_t rec_len;
	uint8_t name_len;
	uint8_t file_type;
	uint8_t name[];
} __attribute__((packed));

_Static_assert(sizeof(struct ext2_superblock_t) == 1024, "Bad ext2 superblock size");
_Static_assert(sizeof(struct ext2_superblock_t) == SUPERBLOCK_SECTORS * SECTOR_SIZE, "Bad ext2 superblock size");
_Static_assert(sizeof(struct ext2_bg_desc_t) == 32, "Bad ext2 bg descriptor size");
_Static_assert(sizeof(struct ext2_inode_t) == 128, "Bad exte2 inode size");

struct ext2_t {
	uint64_t start_lba;
	uint64_t end_lba;
	struct ext2_superblock_t* superblock;
	struct ext2_bg_desc_t* bgdt;
	struct disk_t* disk;
	uint64_t block_size;
	uint8_t lock;
};

struct ext2_inode_handle_t {
	struct ext2_t* ext2;
	uint64_t inode_index;
	uint64_t seek;
	uint64_t seek_block;
	uint8_t ext2_mode;
};

enum ext2_block_state_t {
	BLOCK_OK,
	BLOCK_SPARSE,
	BLOCK_ERROR
};

static uint8_t label_rootfs[16] = {'r', 'o', 'o', 't', 'f', 's', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static void* read_block(uint64_t block, struct ext2_t* ext2) {
	const uint64_t block_size = ext2->block_size;

	void* buffer = kmalloc(block_size);

	const uint64_t lba = ext2->start_lba + block * block_size / SECTOR_SIZE;

	if (lba > ext2->end_lba) {
		logging_log_error("Attempt to read beyond ext2 end lba (0x%llx > 0x%llx)", lba, ext2->end_lba);
		return 0;
	}

	if (disk_read(ext2->disk, buffer, lba, (uint16_t)(block_size / SECTOR_SIZE)) != DISK_OK) {
		logging_log_error("Failed to read");
		kfree(buffer);
		return 0;
	}

	return buffer;
}

static void* write_block_free(uint64_t block, struct ext2_t* ext2, void* buffer, uint8_t free) {
	const uint64_t block_size = ext2->block_size;

	const uint64_t lba = ext2->start_lba + block * block_size / SECTOR_SIZE;

	if (lba > ext2->end_lba) {
		logging_log_error("Attempt to write beyond ext2 end lba (0x%x > 0x%x)", lba, ext2->end_lba);
		if (free) {
			kfree(buffer);
		}
		return 0;
	}

	if (disk_write(ext2->disk, buffer, lba, (uint16_t)(block_size / SECTOR_SIZE)) != DISK_OK) {
		logging_log_error("Failed to write");
		if (free) {
			kfree(buffer);
		}
		return 0;
	}

	return buffer;
}

static inline void* write_block(uint64_t block, struct ext2_t* ext2, void* buffer) {
	return write_block_free(block, ext2, buffer, 1);
}

static uint8_t get_inode(const struct ext2_inode_handle_t* inode_handle, struct ext2_inode_t* inode) {
	const struct ext2_superblock_t* superblock = inode_handle->ext2->superblock;
	const struct ext2_bg_desc_t* bgdt = inode_handle->ext2->bgdt;

	const uint64_t block_size = inode_handle->ext2->block_size;
	const uint64_t block_group = (inode_handle->inode_index - 1) / superblock->s_inodes_per_group;
	const uint64_t lcl_inode_idx = (inode_handle->inode_index - 1) % superblock->s_inodes_per_group;
	const uint64_t lcl_inode_off = lcl_inode_idx * superblock->s_inode_size;
	const uint64_t lcl_inode_blk = lcl_inode_off / block_size;
	const uint64_t inode_off = lcl_inode_off % block_size;

	const uint64_t inode_table_block = bgdt[block_group].bg_inode_table + lcl_inode_blk;
	void* buffer = read_block(inode_table_block, inode_handle->ext2);

	if (!buffer) {
		logging_log_error("Failed to read inode %lu", inode_handle->inode_index);
		kfree(buffer);
		return 1;
	}

	*inode = *(struct ext2_inode_t*)((uint64_t)buffer + inode_off);
	kfree(buffer);
	return 0;
}

static uint8_t set_inode(const struct ext2_inode_handle_t* inode_handle, struct ext2_inode_t* inode) {
	const struct ext2_superblock_t* superblock = inode_handle->ext2->superblock;
	const struct ext2_bg_desc_t* bgdt = inode_handle->ext2->bgdt;

	const uint64_t block_size = inode_handle->ext2->block_size;
	const uint64_t block_group = (inode_handle->inode_index - 1) / superblock->s_inodes_per_group;
	const uint64_t lcl_inode_idx = (inode_handle->inode_index - 1) % superblock->s_inodes_per_group;
	const uint64_t lcl_inode_off = lcl_inode_idx * superblock->s_inode_size;
	const uint64_t lcl_inode_blk = lcl_inode_off / block_size;
	const uint64_t inode_off = lcl_inode_off % block_size;

	const uint64_t inode_table_block = bgdt[block_group].bg_inode_table + lcl_inode_blk;
	void* buffer = read_block(inode_table_block, inode_handle->ext2);

	if (!buffer) {
		logging_log_error("Failed to read inode %lu", inode_handle->inode_index);
		return 1;
	}

	*(struct ext2_inode_t*)((uint64_t)buffer + inode_off) = *inode;

	if (!(buffer = write_block(inode_table_block, inode_handle->ext2, buffer))) {
		logging_log_error("Failed to write inode %lu", inode_handle->inode_index);
		return 1;
	}

	kfree(buffer);
	return 0;
}

static void sync_meta(const struct ext2_t* ext2) {
	// superblock
	disk_write(ext2->disk, ext2->superblock, ext2->start_lba + SUPERBLOCK_LBA, SUPERBLOCK_SECTORS);

	// bgdt
	const uint64_t bgdt_start_lba = ext2->start_lba + (1u << (1 + ext2->superblock->s_log_block_size));
	uint64_t bgdt_size = (1 + ext2->superblock->s_blocks_count / ext2->superblock->s_blocks_per_group)
													* sizeof(struct ext2_bg_desc_t);
	const uint64_t adj = bgdt_size % SECTOR_SIZE;
	if (adj) {
		bgdt_size += SECTOR_SIZE - adj;
	}

	disk_write(ext2->disk, ext2->bgdt, bgdt_start_lba, (uint16_t)(bgdt_size / SECTOR_SIZE));
}

static uint32_t alloc_block(struct ext2_t* ext2, uint64_t group) {
	const uint64_t start_group = group;
	const uint64_t num_groups = ext2->superblock->s_inodes_count / ext2->superblock->s_inodes_per_group;
	const uint64_t bitmap_bytes = ext2->superblock->s_blocks_per_group / 8;

	do {
		if (ext2->bgdt[group].bg_free_blocks_count) {
			void* bitmap = 0;
			for (uint64_t off = 0; off < bitmap_bytes; off += sizeof(uint64_t)) {
				if (off % ext2->block_size == 0) {
					kfree(bitmap);

					bitmap = read_block(ext2->bgdt[group].bg_block_bitmap + (off / ext2->block_size), ext2);

					if (!bitmap) {
						// try next block on failure
						off += ext2->block_size - sizeof(uint64_t);
						continue;
					}
				}

				uint64_t word = ~*(uint64_t*)((uint64_t)bitmap + (off % ext2->block_size));
				if (word) {
					for (uint8_t bit = 0; bit < 64; bit++) {
						if (word & (1uLL << bit)) {
							*(uint64_t*)((uint64_t)bitmap + (off % ext2->block_size)) = ~word | (1uLL << bit);
							ext2->bgdt[group].bg_free_blocks_count--;
							ext2->superblock->s_free_blocks_count--;

							bitmap = write_block(ext2->bgdt[group].bg_block_bitmap + (off / ext2->block_size), ext2, bitmap);

							sync_meta(ext2);

							kfree(bitmap);
							return (uint32_t)((group * ext2->superblock->s_blocks_per_group) + off * 8 + bit);
						}
					}
				}
			}
		}

		if (++group == num_groups - 1) { // TODO: support allocating from last group
			group = 0;
		}
	} while (start_group != group);

	return 0;  // out of blocks
}

static uint64_t alloc_inode(struct ext2_t* ext2, uint64_t group) {
	const uint64_t start_group = group;
	const uint64_t num_groups = ext2->superblock->s_inodes_count / ext2->superblock->s_inodes_per_group;
	const uint64_t bitmap_bytes = ext2->superblock->s_inodes_per_group / 8;

	do {
		if (ext2->bgdt[group].bg_free_inodes_count) {
			void* bitmap = 0;
			for (uint64_t off = 0; off < bitmap_bytes; off += sizeof(uint64_t)) {
				if (off % ext2->block_size == 0) {
					kfree(bitmap);

					bitmap = read_block(ext2->bgdt[group].bg_inode_bitmap + (off / ext2->block_size), ext2);

					if (!bitmap) {
						// try next block on failure
						off += ext2->block_size - sizeof(uint64_t);
						continue;
					}
				}

				uint64_t word = ~*(uint64_t*)((uint64_t)bitmap + (off % ext2->block_size));
				if (word) {
					for (uint8_t bit = 0; bit < 64; bit++) {
						if (word & (1uLL << bit)) {
							*(uint64_t*)((uint64_t)bitmap + (off % ext2->block_size)) = ~word | (1uLL << bit);
							ext2->bgdt[group].bg_free_inodes_count--;
							ext2->superblock->s_free_inodes_count--;

							bitmap = write_block(ext2->bgdt[group].bg_inode_bitmap + (off / ext2->block_size), ext2, bitmap);

							sync_meta(ext2);

							kfree(bitmap);
							return (uint32_t)((group * ext2->superblock->s_inodes_per_group) + off * 8 + bit + 1);
						}
					}
				}
			}
		}

		if (++group == num_groups - 1) { // TODO: support allocating from last group
			group = 0;
		}
	} while (start_group != group);

	return 0;  // out of inodes
}

static uint32_t alloc_and_zero_block(struct ext2_t* ext2, void* zeros, uint64_t group) {
	uint32_t block = alloc_block(ext2, group);

	if (block) {
		write_block_free(block, ext2, zeros, 0);
	}

	return block;
}

static uint64_t assign_block(struct ext2_inode_handle_t* handle, uint64_t index, struct ext2_inode_t* inode, uint8_t lock) {
	const uint64_t group = (handle->inode_index - 1) / handle->ext2->superblock->s_inodes_per_group;
	const uint64_t blocks_usage = handle->ext2->block_size / 512;

	void* zeros = kmalloc(handle->ext2->block_size);
	kmemset(zeros, 0, handle->ext2->block_size);

	if (!lock) {
		lock_acquire(&handle->ext2->lock);
	}

	if (get_inode(handle, inode)) {
		return 0;
	}

	uint32_t block = 0;

	if (index < DIRECT_BLOCKS) {
		block = inode->i_block[index];
		if (!block) {
			block = alloc_and_zero_block(handle->ext2, zeros, group);
			inode->i_block[index] = block;

			if (block) {
				inode->i_blocks += blocks_usage;
			}
		}
		
		goto cleanup;
	}

	index -= DIRECT_BLOCKS;

	uint32_t* buffer;
	const uint64_t block_size = handle->ext2->block_size;
	const uint64_t indir1 = block_size / sizeof(uint32_t);
	uint64_t old_block;

	if (index < indir1) {
		block = inode->i_block[INDIR_1];

		if (!block) {
			block = alloc_and_zero_block(handle->ext2, zeros, group);

			if (!block) {
				goto cleanup;
			}

			inode->i_blocks += blocks_usage;

			inode->i_block[INDIR_1] = block;
		}


		old_block = block;
		buffer = read_block(block, handle->ext2);

		if (!buffer) {
			block = 0;
			goto cleanup;
		}

		block = buffer[index];

		if (!block) {
			block = alloc_and_zero_block(handle->ext2, zeros, group);
			buffer[index] = block;
			buffer = write_block(old_block, handle->ext2, buffer);

			if (block) {
				inode->i_blocks += blocks_usage;
			}
		}

		kfree(buffer);

		goto cleanup;
	}

	index -= indir1;

	const uint64_t indir2 = indir1 * indir1;

	if (index < indir2) {
		block = inode->i_block[INDIR_2];

		if (!block) {
			block = alloc_and_zero_block(handle->ext2, zeros, group);

			if (!block) {
				goto cleanup;
			}

			inode->i_blocks += blocks_usage;

			inode->i_block[INDIR_2] = block;
		}

		old_block = block;
		buffer = read_block(block, handle->ext2);

		if (!buffer) {
			block = 0;
			goto cleanup;
		}

		block = buffer[index / indir1];

		if (!block) {
			block = alloc_and_zero_block(handle->ext2, zeros, group);
			buffer[index / indir1] = block;
			buffer = write_block(old_block, handle->ext2, buffer);

			if (block) {
				inode->i_blocks += blocks_usage;
			}
		}

		kfree(buffer);

		if (!block) {
			goto cleanup;
		}

		old_block = block;
		buffer = read_block(block, handle->ext2);

		if (!buffer) {
			block = 0;
			goto cleanup;
		}

		block = buffer[index % indir1];

		if (!block) {
			block = alloc_and_zero_block(handle->ext2, zeros, group);
			buffer[index % indir1] = block;
			buffer = write_block(old_block, handle->ext2, buffer);

			if (block) {
				inode->i_blocks += blocks_usage;
			}
		}

		kfree(buffer);

		goto cleanup;
	}

	index -= indir2;

	const uint64_t indir3 = indir2 * indir1;

	if (index < indir3) {
		block = inode->i_block[INDIR_3];

		if (!block) {
			block = alloc_and_zero_block(handle->ext2, zeros, group);

			if (!block) {
				goto cleanup;
			}

			inode->i_blocks += blocks_usage;

			inode->i_block[INDIR_3] = block;
		}

		old_block = block;
		buffer = read_block(block, handle->ext2);

		if (!buffer) {
			block = 0;
			goto cleanup;
		}

		block = buffer[index / indir2];

		if (!block) {
			block = alloc_and_zero_block(handle->ext2, zeros, group);
			buffer[index / indir2] = block;
			buffer = write_block(old_block, handle->ext2, buffer);

			if (block) {
				inode->i_blocks += blocks_usage;
			}
		}

		kfree(buffer);

		if (!block) {
			goto cleanup;
		}

		old_block = block;
		buffer = read_block(block, handle->ext2);

		if (!buffer) {
			block = 0;
			goto cleanup;
		}

		block = buffer[(index % indir2) / indir1];

		if (!block) {
			block = alloc_and_zero_block(handle->ext2, zeros, group);
			buffer[(index % indir2) / indir1] = block;
			buffer = write_block(old_block, handle->ext2, buffer);

			if (block) {
				inode->i_blocks += blocks_usage;
			}
		}

		kfree(buffer);

		if (!block) {
			goto cleanup;
		}

		old_block = block;
		buffer = read_block(block, handle->ext2);

		if (!buffer) {
			block = 0;
			goto cleanup;
		}

		block = buffer[index % indir1];

		if (!block) {
			block = alloc_and_zero_block(handle->ext2, zeros, group);
			buffer[index % indir1] = block;
			buffer = write_block(old_block, handle->ext2, buffer);

			if (block) {
				inode->i_blocks += blocks_usage;
			}
		}

		kfree(buffer);

		goto cleanup;	
	}

cleanup:
	set_inode(handle, inode);

	if (!lock) {
		lock_release(&handle->ext2->lock);
	}
	kfree(zeros);

	return block;
}

static enum ext2_block_state_t get_block(struct ext2_t* ext2,
																	struct ext2_inode_t* inode,
																	uint64_t index,
																	uint64_t* block) {
	if (index < DIRECT_BLOCKS) {
		*block = inode->i_block[index];
		if (!*block) {
			return BLOCK_SPARSE;
		}
		return BLOCK_OK;
	}

	index -= DIRECT_BLOCKS;

	uint32_t* buffer;
	const uint64_t block_size = ext2->block_size;
	const uint64_t indir1 = block_size / sizeof(uint32_t);

	if (index < indir1) {
		*block = inode->i_block[INDIR_1];

		if (!*block) {
			return BLOCK_SPARSE;
		}

		buffer = read_block(*block, ext2);

		if (!buffer) {
			return BLOCK_ERROR;
		}

		*block = buffer[index];
		kfree(buffer);

		if (!*block) {
			return BLOCK_SPARSE;
		}

		return BLOCK_OK;
	}

	index -= indir1;

	const uint64_t indir2 = indir1 * indir1;

	if (index < indir2) {
		*block = inode->i_block[INDIR_2];

		if (!*block) {
			return BLOCK_SPARSE;
		}

		buffer = read_block(*block, ext2);

		if (!buffer) {
			return BLOCK_ERROR;
		}

		*block = buffer[index / indir1];
		kfree(buffer);

		if (!*block) {
			return BLOCK_SPARSE;
		}

		buffer = read_block(*block, ext2);

		if (!buffer) {
			return BLOCK_ERROR;
		}

		*block = buffer[index % indir1];
		kfree(buffer);

		if (!*block) {
			return BLOCK_SPARSE;
		}

		return BLOCK_OK;
	}

	index -= indir2;

	const uint64_t indir3 = indir2 * indir1;

	if (index < indir3) {
		*block = inode->i_block[INDIR_3];

		if (!*block) {
			return BLOCK_SPARSE;
		}

		buffer = read_block(*block, ext2);

		if (!buffer) {
			return BLOCK_ERROR;
		}

		*block = buffer[index / indir2];
		kfree(buffer);

		if (!*block) {
			return BLOCK_SPARSE;
		}

		buffer = read_block(*block, ext2);

		if (!buffer) {
			return BLOCK_ERROR;
		}

		*block = buffer[(index % indir2) / indir1];
		kfree(buffer);

		if (!*block) {
			return BLOCK_SPARSE;
		}

		buffer = read_block(*block, ext2);

		if (!buffer) {
			return BLOCK_ERROR;
		}

		*block = buffer[index % indir1];
		kfree(buffer);

		if (!*block) {
			return BLOCK_SPARSE;
		}

		return BLOCK_OK;
	}

	return BLOCK_ERROR;
}

static inline size_t path_entry_len(const char* path) {
	size_t len = 0;

	for (; *path && *path != '/'; path++) {
		len++;
	}

	return len;
}

static uint64_t ext2_get_seek(struct file_handle_t* handle) {
	struct ext2_inode_handle_t* inode_handle = (struct ext2_inode_handle_t*)handle;
	const uint64_t block_size = inode_handle->ext2->block_size;

	return inode_handle->seek_block * block_size + inode_handle->seek;
}

static enum file_status_t ext2_stat(struct file_handle_t* handle, struct file_info_t* info) {
	struct ext2_inode_t inode;
	struct ext2_inode_handle_t* inode_handle = (struct ext2_inode_handle_t*)handle;

	if (!inode_handle || get_inode(inode_handle, &inode)) {
		return FILE_ERROR;
	}

	if (inode.i_mode & EXT2_S_IFREG) {
		info->type = FILE_TYPE_REG;
	}
	else if (inode.i_mode & EXT2_S_IFDIR) {
		info->type = FILE_TYPE_DIR;
	}
	else {
		return FILE_NO_SUPPORT;
	}

	info->size = (uint64_t)inode.i_size | ((uint64_t)inode.i_dir_acl << 32);

	return FILE_OK;
}

static enum file_status_t ext2_open_dir(struct file_handle_t* handle) {
	struct ext2_inode_handle_t* inode_handle = (struct ext2_inode_handle_t*)handle;

	struct file_info_t info;
	if (ext2_stat(handle, &info) != FILE_OK) {
		return FILE_ERROR;
	}

	if (info.type != FILE_TYPE_DIR) {
		return FILE_NOT_DIR;
	}

	inode_handle->seek = 0;
	inode_handle->seek_block = 0;
	inode_handle->ext2_mode |= MODE_DIR;
	return FILE_OK;
}


static void ext2_reset_dir(struct file_handle_t* handle) {
	struct ext2_inode_handle_t* inode_handle = (struct ext2_inode_handle_t*)handle;

	inode_handle->seek = 0;
	inode_handle->seek_block = 0;
	inode_handle->ext2_mode &= ~MODE_DIR;
}

static enum file_status_t ext2_read_dir(struct file_handle_t* handle, struct dir_info_t* info) {
	struct ext2_inode_handle_t* inode_handle = (struct ext2_inode_handle_t*)handle;
	struct ext2_inode_t inode;

	if (!(inode_handle->ext2_mode & MODE_DIR)) {
		return FILE_BAD_FLAGS;
	}

	if (get_inode(inode_handle, &inode)) {
		return FILE_ERROR;
	}

	const uint64_t size = (uint64_t)inode.i_size | ((uint64_t)inode.i_dir_acl << 32);
	uint64_t block;

	while (1) {
		info->seek_pos = ext2_get_seek(handle);

		if (info->seek_pos >= size) {
			return FILE_DNE;
		}

		switch (get_block(inode_handle->ext2, &inode, inode_handle->seek_block, &block)) {
			case BLOCK_OK:
				void* buffer = read_block(block, inode_handle->ext2);

				if (!buffer) {
					return FILE_ERROR;
				}

				struct ext2_ll_dir_entry_t* entry = (struct ext2_ll_dir_entry_t*)((uint64_t)buffer + inode_handle->seek);

				inode_handle->seek += entry->rec_len;
				if (inode_handle->seek >= inode_handle->ext2->block_size) {
					inode_handle->seek_block++;
					inode_handle->seek = 0;
				}

				if (entry->inode != 0) {
					info->inode_num = entry->inode;
					info->rec_len = entry->rec_len;

					switch (entry->file_type) {
						case EXT2_FT_REG_FILE:
							info->type = FILE_INFO_REG;
							break;
						case EXT2_FT_DIR:
							info->type = FILE_INFO_DIR;
							break;
						default:
							info->type = FILE_INFO_UNK;
							break;
					}

					kmemcpy(info->name, entry->name, entry->name_len);
					info->name[entry->name_len] = 0;

					kfree(buffer);
					return FILE_OK;
				}

				kfree(buffer);
				continue;
			case BLOCK_SPARSE:
				inode_handle->seek_block++;
				inode_handle->seek = 0;
				continue;
			case BLOCK_ERROR:
				return FILE_ERROR;
		}
	}
}

static struct ext2_inode_handle_t* ext2_duplicate(struct ext2_inode_handle_t* handle) {
	struct ext2_inode_handle_t* dup = kmalloc(sizeof(struct ext2_inode_handle_t));

	*dup = *handle;
	return dup;
}

static void ext2_close(struct file_handle_t* handle) {
	kfree(handle);
}

static const char* reduce_path(struct ext2_t* ext2, const char* path, struct ext2_inode_handle_t** handle_ret) {
	size_t path_len;
	uint8_t cntrl = 0;

	struct ext2_inode_handle_t* handle = kmalloc(sizeof(struct ext2_inode_handle_t));
	handle->ext2 = ext2;
	handle->inode_index = EXT2_ROOT_INO;
	handle->ext2_mode = 0;

	struct dir_info_t info;

	while (*path) {
		if (ext2_open_dir((struct file_handle_t*)handle) != FILE_OK) {
			break;
		}

		path_len = path_entry_len(path);

		cntrl = 1;
		while (ext2_read_dir((struct file_handle_t*)handle, &info) == FILE_OK) {
			if (path_len == kstrlen(info.name) && kmemcmp(path, info.name, path_len) == 0) {
				cntrl = 0;
				ext2_reset_dir((struct file_handle_t*)handle);
				handle->inode_index = info.inode_num;
				break;
			}
		}

		if (cntrl) {
			break; // file not found
		}

		path += path_len;

		// handle non EOS
		if (*path == '/') {
			path++;
		}
	}

	ext2_reset_dir((struct file_handle_t*)handle);
	*handle_ret = handle;

	return path;
}

static enum file_status_t ext2_create(struct ext2_t* ext2, const char* path, uint32_t mode) {
	(void)mode;

	enum file_status_t sts;
	struct ext2_inode_handle_t* handle;

	path = reduce_path(ext2, path, &handle);

	if (!*path) {
		return FILE_BUSY;
	}

	const char* name = path;

	while (*path && *path != '/') {
		path++;
	}

	if (*path == '/') {
		return FILE_DNE;
	}

	struct file_info_t info;
	if ((sts = ext2_stat((struct file_handle_t*)handle, &info)) != FILE_OK) {
		return sts;
	}

	if (info.type != FILE_TYPE_DIR) {
		return FILE_NO_SUPPORT;
	}

	struct dir_info_t dir_info;
	struct ext2_inode_handle_t* inode_handle = ext2_duplicate((struct ext2_inode_handle_t*)handle);

	lock_acquire(&ext2->lock);

	ext2_open_dir((struct file_handle_t*)inode_handle);

	while (ext2_read_dir((struct file_handle_t*)inode_handle, &dir_info) == FILE_OK) {
		if (kstrcmp(dir_info.name, name) == 0) {
			ext2_reset_dir((struct file_handle_t*)inode_handle);

			lock_release(&inode_handle->ext2->lock);

			ext2_close((struct file_handle_t*)inode_handle);
			
			return FILE_BUSY;
		}
	}

	const uint64_t group = (inode_handle->inode_index - 1) / inode_handle->ext2->superblock->s_inodes_per_group;
	uint64_t inode_index = alloc_inode(inode_handle->ext2, group);
	struct ext2_inode_handle_t* parent_handle = ext2_duplicate((struct ext2_inode_handle_t*)handle);

	sts = FILE_OK;
	if (inode_index == 0) {
		sts = FILE_ERROR;
		goto cleanup;
	}

	struct ext2_inode_t parent_inode;

	if (get_inode(parent_handle, &parent_inode)) {
		sts = FILE_ERROR;
		goto cleanup;
	}

	struct ext2_inode_t inode = {
		.i_mode = EXT2_S_IFREG,
		.i_uid = 0,
		.i_size = 0,
		.i_atime = 0,
		.i_ctime = 0,
		.i_mtime = 0,
		.i_dtime = 0,
		.i_gid = 0,
		.i_links_count = 1,
		.i_blocks = 0,
		.i_flags = 0,
		.i_osd1 = 0,
		.i_block = {
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
		},
		.i_generation = 0,
		.i_file_acl = 0,
		.i_dir_acl = 0,
		.i_faddr = 0,
		.i_osd2 = {
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
		}
	};

	inode_handle->inode_index = inode_index;

	set_inode(inode_handle, &inode);

	uint64_t block;
	ext2_open_dir((struct file_handle_t*)parent_handle);
	//TODO: allocate on an already used block
	while (1) {
		switch (get_block(parent_handle->ext2, &parent_inode, parent_handle->seek_block, &block)) {
			case BLOCK_SPARSE:
				block = assign_block(parent_handle, parent_handle->seek_block, &parent_inode, 1);

				if (!block) {
					sts = FILE_ERROR;
					goto cleanup;
				}

				struct ext2_ll_dir_entry_t* dir_entry = read_block(block, parent_handle->ext2);
				*dir_entry = (struct ext2_ll_dir_entry_t){
					.inode = (uint32_t)inode_index,
					.rec_len = (uint16_t)parent_handle->ext2->block_size,
					.name_len = (uint8_t)kstrlen(name),
					.file_type = EXT2_FT_REG_FILE
				};

				kmemcpy(&dir_entry->name, name, dir_entry->name_len);

				dir_entry = write_block(block, parent_handle->ext2, dir_entry);
				kfree(dir_entry);

				parent_inode.i_size += parent_handle->ext2->block_size;
				set_inode(parent_handle, &parent_inode);
				goto cleanup;
			case BLOCK_OK:
				parent_handle->seek_block++;
				continue;
			case BLOCK_ERROR:
				sts = FILE_ERROR;
				goto cleanup;	
		}
	}

cleanup:
	ext2_reset_dir((struct file_handle_t*)inode_handle);

	lock_release(&inode_handle->ext2->lock);

	ext2_close((struct file_handle_t*)inode_handle);
	ext2_close((struct file_handle_t*)parent_handle);

	return sts;
}

static struct file_handle_t* ext2_open(struct mount_cntx_t* cntx, const char* path, uint32_t flags, uint32_t mode) {
	struct ext2_t* ext2 = (struct ext2_t*)cntx;
	struct ext2_inode_handle_t* handle;

	path = reduce_path(ext2, path, &handle);

	if (*path) {
		// file not found
		kfree(handle);

		if (flags & FILE_FLAGS_CREATE) {
			enum file_status_t create_sts = ext2_create(ext2, path, mode);
			if (create_sts == FILE_OK) {
				return ext2_open(cntx, path, flags & FILE_FLAGS_CREATE, mode);
			}
		}
		return 0;
	}

	return (struct file_handle_t*)handle;
}

static size_t ext2_read(struct file_handle_t* handle, void* buffer, size_t count) {
	struct ext2_inode_t inode;
	struct ext2_inode_handle_t* inode_handle = (struct ext2_inode_handle_t*)handle;
	uint8_t* block_buffer = 0;
	uint64_t block;

	if (!inode_handle || get_inode(inode_handle, &inode)) {
		return 0;
	}

	const uint64_t size = (uint64_t)inode.i_size | ((uint64_t)inode.i_dir_acl << 32);
	const uint64_t block_size = inode_handle->ext2->block_size;

	size_t read = 0;
	uint64_t write_seek = 0;
	size_t read_len;
	uint64_t full_seek = ext2_get_seek(handle);


	while (count) {
		if (full_seek >= size) {
			break;
		}

		read_len = block_size; // default full block

		if (count < block_size) {
			// read partial, only requested
			read_len = count;
		}

		if (read_len + inode_handle->seek > block_size) {
			// read partial, not past block
			read_len = block_size - inode_handle->seek;
		}

		switch (get_block(inode_handle->ext2, &inode, inode_handle->seek_block, &block)) {
			case BLOCK_OK:
				block_buffer = read_block(block, inode_handle->ext2);

				if (!block_buffer) {
					return read;
				}

				if (read_len + full_seek >= size) {
					// full block read, but only copy up to limit of the file
					read_len = size - full_seek;
					count = 0;
				}

				kmemcpy((uint8_t*)buffer + write_seek, block_buffer + inode_handle->seek, read_len);

				kfree(block_buffer);
				break;
			case BLOCK_SPARSE:
				kmemset((uint8_t*)buffer + write_seek, 0, read_len);
				break;
			case BLOCK_ERROR:
				return read;
		}

		write_seek += read_len;
		full_seek += read_len;
		inode_handle->seek += read_len;
		count -= read_len;
		read += read_len;

		if (inode_handle->seek == block_size) {
			inode_handle->seek = 0;
			inode_handle->seek_block++;
		}
	}

	return read;
}

static enum file_status_t ext2_seek(struct file_handle_t* handle, uint64_t seek) {
	struct ext2_inode_handle_t* inode_handle = (struct ext2_inode_handle_t*)handle;
	const uint64_t block_size = inode_handle->ext2->block_size;

	inode_handle->seek_block = seek / block_size;
	inode_handle->seek = seek % block_size;

	return FILE_OK;
}

static size_t ext2_write(struct file_handle_t* handle, const void* buffer, size_t count) {
	struct ext2_inode_t inode;
	struct ext2_inode_handle_t* inode_handle = (struct ext2_inode_handle_t*)handle;
	uint8_t* block_buffer = 0;
	uint64_t block;

	if (!inode_handle || get_inode(inode_handle, &inode)) {
		return 0;
	}

	const uint64_t size = (uint64_t)inode.i_size | ((uint64_t)inode.i_dir_acl << 32);
	const uint64_t block_size = inode_handle->ext2->block_size;

	size_t written = 0;
	uint64_t read_seek = 0;
	size_t write_len;
	uint64_t full_seek = ext2_get_seek(handle);


	while (count) {
		write_len = block_size; // default full block

		if (count < block_size) {
			// write partial, only requested
			write_len = count;
		}

		if (write_len + inode_handle->seek > block_size) {
			// write partial, not past block
			write_len = block_size - inode_handle->seek;
		}

		switch (get_block(inode_handle->ext2, &inode, inode_handle->seek_block, &block)) {
			case BLOCK_SPARSE:
				block = assign_block(inode_handle, inode_handle->seek_block, &inode, 0);

				if (!block) {
					goto update_inode;
				}

				__attribute__((fallthrough));
			case BLOCK_OK:
				block_buffer = read_block(block, inode_handle->ext2);

				if (!block_buffer) {
					goto update_inode;
				}

				kmemcpy(block_buffer + inode_handle->seek, (const uint8_t*)buffer + read_seek, write_len);

				block_buffer = write_block(block, inode_handle->ext2, block_buffer);

				if (!block_buffer) {
					goto update_inode;
				}

				kfree(block_buffer);
				break;
			case BLOCK_ERROR:
				goto update_inode;
		}

		read_seek += write_len;
		full_seek += write_len;
		inode_handle->seek += write_len;
		count -= write_len;
		written += write_len;

		if (inode_handle->seek == block_size) {
			inode_handle->seek = 0;
			inode_handle->seek_block++;
		}
	}

update_inode:
	if (full_seek > size) {
		inode.i_size = (uint32_t)full_seek;
		inode.i_dir_acl = (uint32_t)(full_seek >> 32);

		set_inode(inode_handle, &inode);
	}

	return written;
}

static void ext2_delete_final(struct file_handle_t* handle) {
	(void)handle;
}

static enum file_status_t ext2_create_dir(struct file_handle_t* handle) {
	(void)handle;

	return FILE_NO_SUPPORT;
}

static enum file_status_t ext2_delete_dir(struct file_handle_t* handle) {
	(void)handle;

	return FILE_NO_SUPPORT;
}

static enum file_status_t ext2_truncate(struct file_handle_t* handle, size_t size) {
	(void)handle;
	(void)size;

	return FILE_NO_SUPPORT;
}

static enum file_status_t ext2_link(struct file_handle_t* handle, struct file_handle_t* replace) {
	(void)handle;
	(void)replace;

	return FILE_NO_SUPPORT;
}

static enum file_status_t ext2_unlink(struct file_handle_t* handle) {
	(void)handle;

	return FILE_NO_SUPPORT;
}

uint8_t ext2_attempt_init(struct disk_t* disk, uint64_t start_lba, uint64_t end_lba) {
	struct ext2_superblock_t* superblock = kmalloc(sizeof(struct ext2_superblock_t));
	struct ext2_bg_desc_t* bgdt;
	uint64_t bgdt_size, adj, bgdt_start_lba;

	if (disk_read(disk, superblock, start_lba + SUPERBLOCK_LBA, SUPERBLOCK_SECTORS) != DISK_OK) {
		logging_log_error("Failed to read disk %lu @ %u/%u", disk_get_id(disk), SUPERBLOCK_LBA, SUPERBLOCK_SECTORS);
		kfree(superblock);
		return 0;
	}

	if (superblock->s_magic != EXT2_SUPER_MAGIC) {
		kfree(superblock);
		return 0;
	}

	bgdt_start_lba = start_lba + (1u << (1 + superblock->s_log_block_size));

	bgdt_size = (1 + superblock->s_blocks_count / superblock->s_blocks_per_group) * sizeof(struct ext2_bg_desc_t);
	adj = bgdt_size % SECTOR_SIZE;
	if (adj) {
		bgdt_size += SECTOR_SIZE - adj;
	}

	bgdt = kmalloc(bgdt_size);
	if (disk_read(disk, bgdt, bgdt_start_lba, (uint16_t)(bgdt_size / SECTOR_SIZE)) != DISK_OK) {
		logging_log_error("Failed to read disk %lu @ %u/%u", disk_get_id(disk), SUPERBLOCK_LBA, SUPERBLOCK_SECTORS);
		kfree(superblock);
		kfree(bgdt);
		return 0;
	}

	logging_log_info("Found ext2 filesystem %16.16s", superblock->s_volume_name);

	struct ext2_t* ext2 = kmalloc(sizeof(struct ext2_t));
	ext2->start_lba = start_lba;
	ext2->end_lba = end_lba;
	ext2->superblock = superblock;
	ext2->bgdt = bgdt;
	ext2->disk = disk;
	ext2->block_size = 1024u << superblock->s_log_block_size;
	lock_init(&ext2->lock);

	logging_log_debug("ext2 blocks: 0x%x x 0x%x (0x%lX)",
			1024u << superblock->s_log_block_size, superblock->s_blocks_count,
			(uint64_t)(1024u << superblock->s_log_block_size) * (uint64_t)superblock->s_blocks_count);

	if (!kmemcmp(label_rootfs, superblock->s_volume_name, sizeof(superblock->s_volume_name))) {
		if (fs_mount(
					"/",
					(struct mount_cntx_t*)ext2,
					ext2_open,
					ext2_close,
					ext2_stat,
					ext2_read,
					ext2_get_seek,
					ext2_seek,
					ext2_write,
					ext2_delete_final,
					ext2_open_dir,
					ext2_read_dir,
					ext2_create_dir,
					ext2_delete_dir,
					ext2_truncate,
					ext2_link,
					ext2_unlink
					) != FILE_OK) {
			logging_log_error("Failed to mount rootfs");
			panic(PANIC_STATE);
		}

		scheduler_schedule(process_from_func(prepare_userland, 0));
	}

	//TODO: mount other volumes

	return 1;
}
