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

static const char* panicstr_st = "PANIC: 0x";
static char panicstr[sizeof(panicstr_st) + 256 + 2]; //no crlf, just 0

static const char* panicmsgs[] = {
	": UNKOWN\r\n",
	": OUT OF MEMORY\r\n"};

__attribute__((noreturn)) void panic(uint64_t err) {
	uint64_t offset = 255 - intToString(err, 16, panicstr + sizeof(panicstr_st) - 1, 256);
	kstrcpy(panicstr_st, panicstr + offset);
	panicstr[sizeof(panicstr_st) + 256 + 0] = 0;

	if (err > KPANIC_MAX) {
		err = KPANIC_UNK;
	}

	serialWriteStr(SERIAL1, panicstr + offset);
	serialWriteStr(SERIAL1, panicmsgs[err]);
	serialWriteStr(SERIAL2, panicstr + offset);
	serialWriteStr(SERIAL2, panicmsgs[err]);
	
	panic_hlt();
}

#endif /* CORE_PANIC_C */

