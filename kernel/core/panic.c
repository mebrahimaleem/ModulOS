/* panic.h - kernel panic method */
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

#include <stdint.h>

#include <core/panic.h>
#include <core/cpu_instr.h>

#ifdef SERIAL
#include <drivers/serial/serial_print.h>
#endif /* SERIAL */

static const char* panic_names[] = {
	[PANIC_UNK] = "Unkown",
	[PANIC_PAGING] = "Paging error",
	[PANIC_NO_MEM] = "Out of memory",
	[PANIC_STATE] = "Illegal kernel state",
	[PANIC_MAX] = 0
};

void panic(enum panic_code_t code) {
	const char* name = panic_names[code] ? panic_names[code] : panic_names[PANIC_UNK];
#ifdef SERIAL
	serial_printf_com1("PANIC! 0x%X64 %s\r\nHalt", (uint64_t)code, name);
	serial_printf_com2("PANIC! 0x%X64 %s\r\nHalt", (uint64_t)code, name);
#else
	(void)code;
#endif /* SERIAL */

	cpu_halt_loop();
}
