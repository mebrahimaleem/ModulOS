/* panic.h - kernel panic functions */
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

#include <stdint.h>

#define KPANIC_UNK		0x0
#define KPANIC_NOMEM	0x1
#define KPANIC_ACPI		0x2
#define KPANIC_APIC		0x3
#define KPANIC_MAX		0x3

__attribute__((noreturn)) void panic_hlt(void);

__attribute__((noreturn)) void panic(uint64_t err);

__attribute__((noreturn)) void panicmsg(const char* msg);

#endif /* CORE_PANIC_H */
