/* hpet.c - hpet init */
/* Copyright (C) 2025  Ebrahim Aleem
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

#include <hpet/hpet_init.h>
#include <acpi/tables.h>

#include <kernel/core/timers.h>
#include <kernel/core/logging.h>

timer_init_t default_timer_init = hpet_init;

static uint64_t hpet_reg_base;

void hpet_init(void) {
	hpet_reg_base = acpi_get_hpet_reg_base();
	logging_log_debug("Found HPET register base @ 0x%x64", hpet_reg_base);
}
