/* time.c - time management interface */
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

#ifndef KERNEL_CORE_TIME_H
#define KERNEL_CORE_TIME_H

#include <stdint.h>

extern void time_init(void);

extern uint64_t time_busy_wait(uint64_t min_ns);

extern uint64_t time_since_init_ns(void);

#endif /* KERNEL_CORE_TIME_H */
