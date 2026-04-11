/* disk.h - Disk Access interface */
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

#ifndef DRIVERS_DISK_DISK_H
#define DRIVERS_DISK_DISK_H

#include <stdint.h>
#include <stddef.h>

#define DISK_ID_FIRST	0

#define SECTOR_SIZE	512

enum disk_error_t {
	DISK_OK,
	DISK_ERROR
};

struct disk_t;

typedef enum disk_error_t (*disk_lba_read_t)(void* cntx, void* buffer, uint64_t lba, uint16_t count);
typedef enum disk_error_t (*disk_lba_write_t)(void* cntx, void* pbuffer, uint64_t lba, uint16_t count);
typedef enum disk_error_t (*disk_flush_t)(void* cntx);

extern void disk_init(void);

extern struct disk_t* disk_add(void* cntx, disk_lba_read_t read, disk_lba_write_t write, disk_flush_t flush);

extern struct disk_t* disk_get(uint64_t id);

extern enum disk_error_t disk_read(struct disk_t* disk, void* pbuffer, uint64_t lba, uint16_t count);
extern enum disk_error_t disk_write(struct disk_t* disk, void* pbuffer, uint64_t lba, uint16_t count);
extern enum disk_error_t disk_flush(struct disk_t* disk);

extern uint64_t disk_get_id(struct disk_t* disk);

#endif /* DRIVERS_DISK_DISK_H */
