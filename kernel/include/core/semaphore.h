/* semaphore.h - semaphore interface */
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

#ifndef KERNEL_CORE_SEMAPHORE_H
#define KERNEL_CORE_SEMAPHORE_H

#include <stdint.h>
#include <stddef.h>

#define SEMAPHORE_CAP_UNLIM		(uint64_t)(-1)

struct semaphore_t;

extern struct semaphore_t* semaphore_alloc(size_t cap);
extern void semaphore_free(struct semaphore_t* sem);

extern void semaphore_wait(struct semaphore_t* sem);
extern void semaphore_signal(struct semaphore_t* sem);

extern void semaphore_wait_full(struct semaphore_t* sem);
extern void semaphore_signal_full(struct semaphore_t* sem);

#endif /* KERNEL_CORE_SEMAPHORE_H */

