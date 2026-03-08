/* disk.c - Disk Access implementation */
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
#include <stddef.h>

#include <disk/disk.h>

#ifdef GPT
#include <gpt/gpt.h>
#endif /* GPT */

#include <kernel/core/lock.h>
#include <kernel/core/alloc.h>

struct disk_t {
	disk_lba_read_t read;
	disk_lba_write_t write;
	disk_flush_t flush;
	void* cntx;
	uint64_t id;
	struct disk_t* next;
};

uint8_t disk_lock;

static struct disk_t* disk_list;
static uint64_t disk_id;

void disk_init(void) {
	lock_init(&disk_lock);

	disk_list = 0;
	disk_id = DISK_ID_FIRST;
}

struct disk_t* disk_add(void* cntx, disk_lba_read_t read, disk_lba_write_t write, disk_flush_t flush) {
	struct disk_t* disk = kmalloc(sizeof(struct disk_t));

	disk->read = read;
	disk->write = write;
	disk->flush = flush;
	disk->cntx = cntx;

	lock_acquire(&disk_lock);
	disk->id = disk_id++;
	disk->next = disk_list;
	disk_list = disk;
	lock_release(&disk_lock);

#ifdef GPT
	if (gpt_find_partitions(disk)) {
		return disk;
	}
#endif /* GPT */

	return disk;
}

struct disk_t* disk_get(uint64_t id) {
	struct disk_t* disk;

	lock_acquire(&disk_lock);
	for (disk = disk_list; disk; disk = disk->next) {
		if (disk->id == id) {
			break;
		}
	}
	lock_release(&disk_lock);

	return disk;
}

enum disk_error_t disk_read(struct disk_t* disk, void* buffer, uint64_t lba, uint16_t count) {
	return disk->read(disk->cntx, buffer, lba, count);
}

enum disk_error_t disk_write(struct disk_t* disk, void* buffer, uint64_t lba, uint16_t count) {
	return disk->write(disk->cntx, buffer, lba, count);
}

enum disk_error_t disk_flush(struct disk_t* disk) {
	return disk->flush(disk->cntx);
}

uint64_t disk_get_id(struct disk_t* disk) {
	return disk->id;
}
