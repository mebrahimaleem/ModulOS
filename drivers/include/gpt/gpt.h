/* gpt.h - GUID partition table driver interface */
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

#ifndef DRIVERS_GPT_GPT_H
#define DRIVERS_GPT_GPT_H

#include <stdint.h>

#include <drivers/disk/disk.h>

extern uint8_t gpt_find_partitions(struct disk_t* disk);

#endif /* DRIVERS_GPT_GPT_H */
