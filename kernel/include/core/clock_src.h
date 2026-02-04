/* clock_src.h - generic clock source interface */
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

#ifndef KERNEL_CORE_CLOCK_SRC_H
#define KERNEL_CORE_CLOCK_SRC_H

#include <stdint.h>

struct clock_src_t {
	uint64_t period_fs;
	uint64_t (*counter)(void*);
	void (*reset)(void*, uint64_t);
	void* meta;
};

extern void clock_src_init(void);

extern void clock_src_register(struct clock_src_t* clock);

extern struct clock_src_t* clock_src_alloc(void);

#endif /* KERNEL_CORE_CLOCK_SRC_H */
