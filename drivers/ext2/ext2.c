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

#define SUPERBLOCK_LBA			2
#define SUPERBLOCK_SECTORS	2

#define EXT2_SUPER_MAGIC	0xEF53

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

_Static_assert(sizeof(struct ext2_superblock_t) == 1024, "Bad ext2 superblock size");
_Static_assert(sizeof(struct ext2_bg_desc_t) == 32, "Bad ext2 bg descriptor size");
_Static_assert(sizeof(struct ext2_inode_t) == 128, "Bad exte2 inode size");

uint8_t ext2_attempt_init(struct disk_t* disk, uint64_t start_lba, uint64_t end_lba) {
	struct ext2_superblock_t* superblock = kmalloc(sizeof(struct ext2_superblock_t));

	(void)end_lba;

	if (disk_read(disk, superblock, start_lba + SUPERBLOCK_LBA, SUPERBLOCK_SECTORS) != DISK_OK) {
		logging_log_error("Failed to read disk %lu @ %u/%u", disk_get_id(disk), SUPERBLOCK_LBA, SUPERBLOCK_SECTORS);
		kfree(superblock);
		return 0;
	}

	if (superblock->s_magic != EXT2_SUPER_MAGIC) {
		kfree(superblock);
		return 0;
	}

	logging_log_info("Found ext2 filesystem %16.16s", superblock->s_volume_name);

	kfree(superblock);
	return 1;
}
