/* clock_src.h - generic clock source utilities */
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

#include <core/clock_src.h>
#include <core/alloc.h>

#include <drivers/hpet/hpet_init.h>

struct clock_src_list_t {
	struct clock_src_t* clock;
	struct clock_src_list_t* next;
};

static struct clock_src_list_t* clocks;

void clock_src_init(void) {
	clocks = 0;

#ifdef HPET
	hpet_init();
#endif /* HPET */
}

void clock_src_register(struct clock_src_t* clock) {
	struct clock_src_list_t* node = kmalloc(sizeof(struct clock_src_list_t));
	node->clock = clock;
	node->next = clocks;
	clocks = node;
}

struct clock_src_t* clock_src_alloc(void) {
	struct clock_src_t* source = 0;

	if (clocks) {
		source = clocks->clock;
		clocks = clocks->next;
		//TODO: free clock
	}

	return source;
}
