/* gpt.c - GUID partition table driver */
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

#include <gpt/gpt.h>

#include <disk/disk.h>

#ifdef EXT2
#include <ext2/ext2.h>
#endif /* EXT2 */

#include <kernel/core/alloc.h>
#include <kernel/core/logging.h>

#include <kernel/lib/kmemcmp.h>
#include <kernel/lib/hash.h>

#define GPT_LBA	1
#define GPT_SIG	"EFI PART"

struct gpt_partition_table_t {
	uint8_t sig[8];
	uint32_t rev;
	uint32_t header_size;
	uint32_t crc32;
	uint32_t resv0;
	uint64_t curr_lba;
	uint64_t alt_lba;
	uint64_t first_block;
	uint64_t last_block;
	uint8_t guid[16];
	uint64_t partition_array_lba;
	uint32_t partition_array_entry_count;
	uint32_t partition_array_entry_size;
	uint32_t partition_array_crc32;
} __attribute__((packed));

struct gpt_partition_entry_t {
	uint8_t part_type[16];
	uint8_t guid[16];
	uint64_t start_lba;
	uint64_t end_lba;
	uint64_t attr;
	uint8_t name[];
} __attribute__((packed));

uint8_t gpt_find_partitions(struct disk_t* disk) {
	struct gpt_partition_table_t* gpt = kmalloc(SECTOR_SIZE);
	uint32_t crc;
	uint64_t partition_array_size;
	void* partition_array_base;
	struct gpt_partition_entry_t* entry;
	uint8_t j;
	uint32_t i;

	if (disk_read(disk, gpt, GPT_LBA, 1) != DISK_OK) {
		logging_log_error("Failed to read gpt lba of disk %lu", disk_get_id(disk));
		kfree(gpt);
		return 0;
	}

	if (kmemcmp(gpt->sig, GPT_SIG, sizeof(gpt->sig))) {
		kfree(gpt);
		return 0;
	}

	logging_log_debug("Found GPT on disk %lu", disk_get_id(disk));

	crc = gpt->crc32;	
	gpt->crc32 = 0;

	if (crc32_ansi(gpt, sizeof(*gpt)) != crc) {
		logging_log_error("Bad checksum for GPT. Skipping disk");
		kfree(gpt);
		return 0;
	}

	partition_array_size = gpt->partition_array_entry_count * gpt->partition_array_entry_size;
	partition_array_base = kmalloc(SECTOR_SIZE * ((partition_array_size / SECTOR_SIZE) + 1));

	if (disk_read(disk, partition_array_base, gpt->partition_array_lba,
				(uint16_t)(partition_array_size / SECTOR_SIZE) + 1) != DISK_OK) {
		logging_log_error("Failed to read GPT partition array. Skipping disk");
		kfree(gpt);
		kfree(partition_array_base);
		return 0;
	}

	if(crc32_ansi((void*)partition_array_base, partition_array_size) != gpt->partition_array_crc32) {
		logging_log_error("Bad checksum for GPT partition array. Skipping disk");
		kfree(gpt);
		kfree(partition_array_base);
		return 0;
	}

	for (i = 0; i < gpt->partition_array_entry_count; i++) {
		entry = (struct gpt_partition_entry_t*)((uint64_t)partition_array_base + i * gpt->partition_array_entry_size);
		for (j = 0; j < 16; j++) {
			if (entry->part_type[j]) {
				logging_log_debug("Found GPT partition on %lu @ %u", disk_get_id(disk), i);

#ifdef EXT2
				if (ext2_attempt_init(disk, entry->start_lba, entry->end_lba)) {
					break;
				}
#endif /* EXT2 */
				break;
			}
		}
	}

	kfree(gpt);
	kfree(partition_array_base);

	return 0;
}
