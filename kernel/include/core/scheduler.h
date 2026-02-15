/* scheduler.h - kernel scheduler interface */
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

#ifndef KERNEL_CORE_SCHEDULER_H
#define KERNEL_CORE_SCHEDULER_H

#include <stdint.h>

#include <kernel/core/process.h>

extern void scheduler_init(void);

extern void scheduler_start(void) __attribute__((noreturn));

extern void scheduler_run(void) __attribute__((noreturn));

extern void scheduler_schedule(struct pcb_t* pcb);

#endif /* KERNEL_CORE_SCHEDULER_H */
