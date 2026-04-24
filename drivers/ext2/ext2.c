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
#include <kernel/core/elf.h>
#include <kernel/core/scheduler.h>

#include <kernel/lib/kmemcmp.h>
#include <kernel/lib/kmemcpy.h>
#include <kernel/lib/kmemset.h>

#define SUPERBLOCK_LBA			2
#define SUPERBLOCK_SECTORS	2

#define DIRECT_BLOCKS			12
#define INDIR_1						12
#define INDIR_2						13
#define INDIR_3						14

#define BLOCK_OK		0
#define BLOCK_RETRY	1
#define BLOCK_LAST	2
#define BLOCK_ERROR	3

#define EXT2_FT_DIR	2

#define EXT2_SUPER_MAGIC	0xEF53

#define EXT2_ROOT_INO			2

#define EXT2_S_IFREG	0x8000
#define EXT2_S_IFDIR	0x4000

#define DIRLS_SET			0x8000000000000000

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
_Static_assert(sizeof(struct ext2_bg_desc_t) == 32, "Bad ext2 bg descriptor size");
_Static_assert(sizeof(struct ext2_inode_t) == 128, "Bad exte2 inode size");

struct ext2_t {
	uint64_t start_lba;
	uint64_t end_lba;
	struct ext2_superblock_t* superblock;
	struct ext2_bg_desc_t* bgdt;
	struct disk_t* disk;
};

struct ext2_inode_handle_t {
	struct ext2_t* ext2;
	uint64_t inode_index;
	uint64_t seek;
	uint64_t seek_block;
};

struct ext2_block_track_t {
	struct ext2_inode_handle_t* handle;
	struct ext2_inode_t* inode;
	uint64_t off;
	uint64_t block;
};

static uint8_t label_rootfs[16] = {'r', 'o', 'o', 't', 'f', 's', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static void* read_block(uint64_t block, struct ext2_t* ext2) {
	const uint64_t block_size = 1024u << ext2->superblock->s_log_block_size;

	void* buffer = kmalloc(block_size);

	const uint64_t lba = ext2->start_lba + block * block_size / SECTOR_SIZE;

	if (lba > ext2->end_lba) {
		logging_log_error("Attempt to read beyond ext2 end lba (0x%x > 0x%x)", lba, ext2->end_lba);
	}

	if (disk_read(ext2->disk, buffer, lba, (uint16_t)(block_size / SECTOR_SIZE)) != DISK_OK) {
		logging_log_error("Failed to read");
		kfree(buffer);
		return 0;
	}

	return buffer;
}

static uint8_t get_inode(const struct ext2_inode_handle_t* inode_handle, struct ext2_inode_t* inode) {
	const struct ext2_superblock_t* superblock = inode_handle->ext2->superblock;
	const struct ext2_bg_desc_t* bgdt = inode_handle->ext2->bgdt;

	const uint64_t block_size = 1024u << superblock->s_log_block_size;
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

static uint8_t get_block_index(struct ext2_block_track_t* bt, uint64_t* index) {
	uint64_t block = bt->block;
	struct ext2_inode_t* inode = bt->inode;
	struct ext2_t* ext2 = bt->handle->ext2;


	if (block < DIRECT_BLOCKS) {
		*index = inode->i_block[block];

		if (!*index) {
			bt->block += 1;
			return BLOCK_RETRY;
		}

		return BLOCK_OK;
	}

	block -= DIRECT_BLOCKS;

	const uint64_t block_size = 1024u << ext2->superblock->s_log_block_size;
	const uint64_t indir1 = block_size / sizeof(uint32_t);

	uint32_t* buffer;

	if (block < indir1) {
		*index = inode->i_block[INDIR_1];

		if (!*index) {
			bt->block += indir1;
			return BLOCK_RETRY;
		}

		buffer = read_block(*index, ext2);

		if (!buffer) {
			return BLOCK_ERROR;
		}

		*index = buffer[block];
		kfree(buffer);

		if (!*index) {
			bt->block += 1;
			return BLOCK_RETRY;
		}

		return BLOCK_OK;
	}

	block -= indir1;

	const uint64_t indir2 = indir1 * indir1;


	if (block < indir2) {
		*index = inode->i_block[INDIR_2];

		if (!*index) {
			bt->block += indir2;
			return BLOCK_RETRY;
		}

		buffer = read_block(*index, ext2);

		if (!buffer) {
			return BLOCK_ERROR;
		}

		*index = buffer[block / indir1];
		kfree(buffer);

		if (!*index) {
			bt->block += indir1;
			return BLOCK_RETRY;
		}

		buffer = read_block(*index, ext2);

		if (!buffer) {
			return BLOCK_ERROR;
		}

		*index = buffer[block % indir1];
		kfree(buffer);

		if (!*index) {
			bt->block += 1;
			return BLOCK_RETRY;
		}

		return BLOCK_OK;
	}

	block -= indir2;

	const uint64_t indir3 = indir2 * indir1;

	if (block < indir3) {
		*index = inode->i_block[INDIR_3];

		if (!*index) {
			bt->block += indir3;
			return BLOCK_RETRY;
		}

		buffer = read_block(*index, ext2);

		if (!buffer) {
			return BLOCK_ERROR;
		}

		*index = buffer[block / indir2];
		kfree(buffer);

		if (!*index) {
			bt->block += indir2;
			return BLOCK_RETRY;
		}

		buffer = read_block(*index, ext2);

		if (!buffer) {
			return BLOCK_ERROR;
		}

		*index = buffer[(block % indir2) / indir1];
		kfree(buffer);

		if (!*index) {
			bt->block += indir1;
			return BLOCK_RETRY;
		}

		buffer = read_block(*index, ext2);

		if (!buffer) {
			return BLOCK_ERROR;
		}

		*index = buffer[block % indir1];
		kfree(buffer);

		if (!*index) {
			bt->block += 1;
			return BLOCK_RETRY;
		}

		return BLOCK_OK;
	}

	return BLOCK_LAST;
}

static inline size_t path_entry_len(char* path) {
	size_t len = 0;

	for (; *path && *path != '/'; path++) {
		len++;
	}

	return len;
}


static struct file_handle_t* ext2_open(struct mount_cntx_t* cntx, char* path) {
	struct ext2_inode_t inode;
	size_t path_len;
	struct ext2_block_track_t track;
	uint64_t block_index;
	uint8_t cntrl = 0;


	track.handle = kmalloc(sizeof(struct ext2_inode_handle_t));

	if (!track.handle) {
		return 0;
	}

	track.handle->ext2 = (struct ext2_t*)cntx;
	track.handle->inode_index = EXT2_ROOT_INO;
	track.handle->seek = 0;
	track.handle->seek_block = 0;

	const struct ext2_superblock_t* superblock = track.handle->ext2->superblock;
	const uint64_t block_size = 1024u << superblock->s_log_block_size;
	const uint64_t size = (uint64_t)inode.i_size | ((uint64_t)inode.i_dir_acl << 32);
	void* buffer;

	while (*path && cntrl != 2) {
		path_len = path_entry_len(path);

		if (get_inode(track.handle, &inode)) {
			return 0;
		}

		track.inode = &inode;
		track.block = 0;
		track.off = 0;

		cntrl = 1;
		while (cntrl == 1) {
			if (track.block * block_size > size) {
				cntrl = 2;
				break;
			}

			if (track.off >= block_size) {
				track.block++;
				track.off = 0;
			}

			switch (get_block_index(&track, &block_index)) {
				case BLOCK_OK:
					buffer = read_block(block_index, track.handle->ext2);
					if (!buffer) {
						kfree(track.handle);
						return 0;
					}

					struct ext2_ll_dir_entry_t* entry = (struct ext2_ll_dir_entry_t*)((uint64_t)buffer + track.off);

					if (entry->inode != 0) {
						if (entry->name_len == path_len && !kmemcmp(entry->name, path, path_len)) {
							path += path_len;

							// handle non EOS
							if (*path == '/') {
								path++;
							}

							track.handle->inode_index = entry->inode;

							if (*path && entry->file_type != EXT2_FT_DIR) {
								cntrl = 2; // file not found
							}
							else {
								cntrl = 0;
							}

							kfree(buffer);
							break;
						}
					}

					track.off += entry->rec_len;
					kfree(buffer);
					__attribute__((fallthrough));
				case BLOCK_RETRY:
					continue;
				case BLOCK_LAST:
					cntrl = 2; // file not found
					break;
				case BLOCK_ERROR:
				default:
					kfree(track.handle);
					return 0;
			}
		}
	}

	if (*path || cntrl == 2) {
		kfree(track.handle); // file not found
		return 0;
	}

	return (struct file_handle_t*)track.handle;
}

static void ext2_close(struct file_handle_t* handle) {
	kfree(handle);
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

static size_t ext2_read(struct file_handle_t* handle, void* buffer, size_t count) {
	struct ext2_inode_t inode;
	struct ext2_inode_handle_t* inode_handle = (struct ext2_inode_handle_t*)handle;
	struct ext2_block_track_t track;
	uint8_t* block_buffer = 0;

	if (!inode_handle || get_inode(inode_handle, &inode)) {
		return 0;
	}

	const uint64_t size = (uint64_t)inode.i_size | ((uint64_t)inode.i_dir_acl << 32);
	const struct ext2_superblock_t* superblock = inode_handle->ext2->superblock;
	const uint64_t block_size = 1024u << superblock->s_log_block_size;

	size_t read = 0;
	uint64_t index;
	uint64_t write_seek = 0;
	size_t write_len;

	while (count) {
		track.block = inode_handle->seek_block;
		track.inode = &inode;
		track.handle = inode_handle;

		if (track.block * block_size > size) {
			break;
		}

		write_len = block_size; // default full block

		if (count < block_size) {
			// read partial, only requested
			write_len = count;
		}

		if (write_len + inode_handle->seek > block_size) {
			// read partial, not past block
			write_len = block_size - inode_handle->seek;
		}

		switch (get_block_index(&track, &index)) {
			case BLOCK_OK:
				block_buffer = read_block(index, inode_handle->ext2);

				if (!block_buffer) {
					return read;
				}

				kmemcpy((uint8_t*)buffer + write_seek, block_buffer + inode_handle->seek, write_len);

				kfree(block_buffer);
				break;
			case BLOCK_RETRY:
				kmemset((uint8_t*)buffer + write_seek, 0, write_len);
				break;
			default:
				return read;
		}

		write_seek += write_len;
		inode_handle->seek += write_len;
		count -= write_len;
		read += write_len;

		if (inode_handle->seek == block_size) {
			inode_handle->seek = 0;
			inode_handle->seek_block++;
		}
	}

	return read;
}

static uint64_t ext2_get_seek(struct file_handle_t* handle) {
	struct ext2_inode_handle_t* inode_handle = (struct ext2_inode_handle_t*)handle;
	const struct ext2_superblock_t* superblock = inode_handle->ext2->superblock;
	const uint64_t block_size = 1024u << superblock->s_log_block_size;

	return inode_handle->seek_block * block_size + inode_handle->seek;
}

static enum file_status_t ext2_seek(struct file_handle_t* handle, uint64_t seek) {
	struct ext2_inode_handle_t* inode_handle = (struct ext2_inode_handle_t*)handle;
	const struct ext2_superblock_t* superblock = inode_handle->ext2->superblock;
	const uint64_t block_size = 1024u << superblock->s_log_block_size;

	inode_handle->seek_block = seek / block_size;
	inode_handle->seek = seek % block_size;

	return FILE_OK;
}

static size_t ext2_write(struct file_handle_t* handle, void* buffer, size_t count) {
	(void)handle;
	(void)buffer;
	(void)count;
	return 0;
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
					ext2_write
					) != FILE_OK) {
			logging_log_error("Failed to mount rootfs");
			panic(PANIC_STATE);
		}

		struct file_info_t stat_buf;
		enum file_status_t sts;

		struct fs_handle_t* root_handle = fs_open("/");
		if (!root_handle) {
			logging_log_error("Failed to open root directory");
		}
		else {
			if ((sts = fs_stat(root_handle, &stat_buf)) != FILE_OK) {
				logging_log_error("Failed to stat root directory %u", (uint32_t)sts);
			}
			else {
				logging_log_debug("Root directory %u (%u)", stat_buf.size, stat_buf.type);
			}

			fs_close(root_handle);
		}


		struct fs_handle_t* shell = fs_open("/bin/shell");
		if (!shell) {
			logging_log_error("Failed to open shell file");
		}
		else {
			struct pcb_t* shell_pcb = elf_load(shell, process_assign_pid(), "/bin/shell ModulOS", "USER=root PWD=/");
			if (!shell_pcb) {
				logging_log_error("Failed to load shell file");
			}
			fs_close(shell);

			scheduler_schedule(shell_pcb);
		}
	}

	//TODO: mount other volumes

	return 1;
}
