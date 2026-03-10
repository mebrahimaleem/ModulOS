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

#include <kernel/lib/kmemcmp.h>
#include <kernel/lib/kmemcpy.h>

#define SUPERBLOCK_LBA			2
#define SUPERBLOCK_SECTORS	2

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
};

struct ext2_dir_track_t {
	struct ext2_inode_handle_t* handle;
	struct ext2_inode_t dir;
	uint64_t off;
};

static uint8_t label_rootfs[16] = {'r', 'o', 'o', 't', 'f', 's', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

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
	void* buffer = kmalloc(block_size);

	if (disk_read(
				inode_handle->ext2->disk,
				buffer,
				inode_handle->ext2->start_lba + inode_table_block * block_size / SECTOR_SIZE,
				(uint16_t)(block_size / SECTOR_SIZE)) != DISK_OK) {
		logging_log_error("Failed to read inode %lu", inode_handle->inode_index);
		kfree(buffer);
		return 1;
	}

	*inode = *(struct ext2_inode_t*)((uint64_t)buffer + inode_off);
	kfree(buffer);
	return 0;
}

static enum fs_status_t ext2_stat(struct fs_file_handle_t* file, struct fs_info_t* info) {
	struct ext2_inode_t inode;
	
	if (get_inode((struct ext2_inode_handle_t*)file, &inode)) {
		return FS_STATUS_ERROR;
	}

	info->size = (uint32_t)inode.i_size | ((uint64_t)inode.i_dir_acl << 32);

	if (inode.i_mode & EXT2_S_IFREG) {
		info->type = FS_TYPE_FILE;
	}
	else if (inode.i_mode & EXT2_S_IFDIR) {
		info->type = FS_TYPE_DIR;
	}
	else {
		return FS_STATUS_ERROR;
	}

	return FS_STATUS_OK;
}

static struct fs_dirls_handle_t* ext2_dirst(struct fs_file_handle_t* handle) {
	struct ext2_inode_t inode;
	
	if (get_inode((struct ext2_inode_handle_t*)handle, &inode)) {
		return 0;
	}

	struct ext2_dir_track_t* dir_track = kmalloc(sizeof(struct ext2_dir_track_t));	
	dir_track->handle = (struct ext2_inode_handle_t*)handle;
	dir_track->dir = inode;
	dir_track->off = 0;
	//TODO: support reading from other blocks as well
	return (struct fs_dirls_handle_t*)dir_track;
}

static enum fs_status_t ext2_dirls(struct fs_dirls_handle_t** handle, struct fs_ls_info_t* file) {
	struct ext2_dir_track_t* dt = (struct ext2_dir_track_t*)*handle;

	const struct ext2_superblock_t* superblock = dt->handle->ext2->superblock;

	const uint64_t block_size = 1024u << superblock->s_log_block_size;

	if (dt->off >= block_size) {
		return FS_STATUS_EOF;
	}

	const uint64_t lba = dt->handle->ext2->start_lba + dt->dir.i_block[0] * block_size / SECTOR_SIZE;

	void* buffer = kmalloc(block_size);

	if (disk_read(dt->handle->ext2->disk, buffer, lba, (uint16_t)(block_size / SECTOR_SIZE)) != DISK_OK) {
		logging_log_error("Failed to read");
	}

	struct ext2_ll_dir_entry_t* entry =
		(struct ext2_ll_dir_entry_t*)((uint64_t)buffer + dt->off);

	if (entry->inode != 0) {
		if (entry->name[0] == '.' && (entry->name_len == 1 || (entry->name_len == 2 && entry->name[1] == '.'))) {
			file->valid = 0;
		}

		else {
			kmemcpy(file->name, entry->name, entry->name_len);
			file->name_len = entry->name_len;
			struct ext2_inode_handle_t* hndl = kmalloc(sizeof(struct ext2_inode_handle_t));
			hndl->ext2 = dt->handle->ext2;
			hndl->inode_index = entry->inode;
			file->handle = (struct fs_file_handle_t*)hndl;
			file->valid = 1;
		}
	}

	dt->off += entry->rec_len;

	kfree(buffer);
	return FS_STATUS_OK;
}

static void ext2_diren(struct fs_dirls_handle_t* handle) {
	kfree(handle);
}

uint8_t ext2_attempt_init(struct disk_t* disk, uint64_t start_lba, uint64_t end_lba) {
	struct ext2_superblock_t* superblock = kmalloc(sizeof(struct ext2_superblock_t));
	struct ext2_inode_handle_t* root_handle;
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

	root_handle = kmalloc(sizeof(struct ext2_inode_handle_t));
	root_handle->ext2 = kmalloc(sizeof(struct ext2_t));
	root_handle->ext2->start_lba = start_lba;
	root_handle->ext2->end_lba = end_lba;
	root_handle->ext2->superblock = superblock;
	root_handle->ext2->bgdt = bgdt;
	root_handle->ext2->disk = disk;
	root_handle->inode_index = EXT2_ROOT_INO;

	logging_log_debug("ext2 blocks: 0x%x x 0x%x (0x%lX)",
			1024u << superblock->s_log_block_size, superblock->s_blocks_count,
			(uint64_t)(1024u << superblock->s_log_block_size) * (uint64_t)superblock->s_blocks_count);

	if (!kmemcmp(label_rootfs, superblock->s_volume_name, sizeof(superblock->s_volume_name))) {
		if (fs_mount((struct fs_file_handle_t*)root_handle, "/",
					ext2_stat,
					ext2_dirst,
					ext2_dirls,
					ext2_diren
					) != FS_STATUS_OK) {
			logging_log_error("Failed to mount rootfs");
			panic(PANIC_STATE);
		}

		struct fs_file_t* root_file = fs_open("/.");
		if (!root_file) {
			logging_log_error("Failed to open root directory");
			panic(PANIC_STATE);
		}

		struct fs_info_t root_file_info;
		if (fs_stat(root_file, &root_file_info) != FS_STATUS_OK) {
			logging_log_error("Failed to stat root directory");
			panic(PANIC_STATE);
		}

		logging_log_debug("Root directory (0x%lx) (%u)", root_file_info.size, root_file_info.type);

		fs_close(root_file);
	}

	fs_log_vfs_tree();

	//TODO: mount other volumes

	return 1;
}
