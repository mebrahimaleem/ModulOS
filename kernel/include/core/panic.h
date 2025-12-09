/* panic.h - kernel panic interface */
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

#ifndef CORE_PANIC_H
#define CORE_PANIC_H

enum panic_code_t {
	PANIC_UNK,
	PANIC_PAGING,
	PANIC_NO_MEM,
};

extern void panic(enum panic_code_t code) __attribute__((noreturn));

#endif /* CORE_PANIC_H */
