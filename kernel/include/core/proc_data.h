/* proc_data.h - per procesor data interface */
/* Copyright (C) 2025-2026  Ebrahim Aleem
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

#ifndef KERNEL_CORE_PROC_DATA_H
#define KERNEL_CORE_PROC_DATA_H

#include <stdint.h>

#include <kernel/core/alloc.h>

struct proc_data_t {
	uint8_t arb_id;
	struct proc_alloc_caches_t alloc_caches;
};

extern struct proc_data_t bsp_proc_data;
extern struct proc_data_t** proc_data_ptr;

extern void proc_data_set_id(uint8_t id);
extern struct proc_data_t* proc_data_get(void);

#endif /* KERNEL_CORE_PROC_DATA_H */
