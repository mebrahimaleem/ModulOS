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

#define KPANIC_UNK		0
#define KPANIC_NOMEM	1

__attribute__((noreturn)) void kpanic_hlt(void);

__attribute__((noreturn)) void kpanic(uint64_t err);

#endif /* CORE_PANIC_H */
