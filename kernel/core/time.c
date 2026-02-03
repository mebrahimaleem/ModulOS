/* time.c - time management utilities */
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

#include <core/time.h>
#include <core/clock_src.h>
#include <core/cpu_instr.h>
#include <core/logging.h>
#include <core/panic.h>

static struct clock_src_t* clock;

void time_init(void) {
	if(!(clock = clock_src_alloc()))  {
		logging_log_error("Out of clock sources");
		panic(PANIC_STATE);
	}

	clock->reset(clock->meta, 0);
}

uint64_t time_busy_wait(uint64_t min_ns) {
	const uint64_t start = clock->counter(clock->meta) * clock->period_fs;
	const uint64_t stop = start + min_ns * TIME_CONV_NS_TO_FS;
	uint64_t actual;
	while ((actual = clock->counter(clock->meta) * clock->period_fs) < stop) {
		cpu_pause();
	}

	return actual - start;
}

uint64_t time_since_init_ns(void) {
	return clock->counter(clock->meta) * clock->period_fs / TIME_CONV_NS_TO_FS;
}
