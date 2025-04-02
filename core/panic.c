/* panic.c - kernel panic functions */
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

#ifndef CORE_PANIC_C
#define CORE_PANIC_C

#include <core/utils.h>
#include <core/serial.h>
#include <core/panic.h>

static const char* panicmsgs[] = {
	"UNKOWN\n",
	"OUT OF MEMORY\n",
	"ACPI ERROR\n",
	"APIC ERROR\n"
};

__attribute__((noreturn)) void panic(uint64_t err) {
	if (err > KPANIC_MAX) {
		err = KPANIC_UNK;
	}

	serialPrintf(SERIAL1, "PANIC: 0x%lX: %s\r\n", (uint64_t)err, panicmsgs[err]);
	serialPrintf(SERIAL2, "PANIC: 0x%lX: %s\r\n", (uint64_t)err, panicmsgs[err]);
	
	panic_hlt();
}

__attribute__((noreturn)) void panicmsg(const char* msg) {
	serialPrintf(SERIAL1, "PANIC: %s\r\n", msg);
	serialPrintf(SERIAL2, "PANIC: %s\r\n", msg);

	panic_hlt();
}

#endif /* CORE_PANIC_C */

