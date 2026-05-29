/* semaphore.c - semaphore implementation */
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

#include <stdint.h>
#include <stddef.h>

#include <core/semaphore.h>
#include <core/lock.h>
#include <core/alloc.h>
#include <core/cpu_instr.h>

struct semaphore_t {
	size_t rem;
	size_t cap;
	uint8_t lock;
};

struct semaphore_t* semaphore_alloc(size_t cap) {
	struct semaphore_t* sem = kmalloc(sizeof(struct semaphore_t));

	sem->rem = sem->cap = cap;
	lock_init(&sem->lock);

	return sem;
}

void semaphore_free(struct semaphore_t* sem) {
	kfree(sem);
}

void semaphore_wait(struct semaphore_t* sem) {
	lock_acquire(&sem->lock);

	while (!sem->rem) {
		cpu_pause();
	}

	sem->rem--;

	lock_release(&sem->lock);
}

void semaphore_signal(struct semaphore_t* sem) {
	sem->rem++;
}

void semaphore_wait_full(struct semaphore_t* sem) {
	lock_acquire(&sem->lock);

	while (sem->rem != sem->cap) {
		cpu_pause();
	}

	sem->rem = 0;

	lock_release(&sem->lock);
}

void semaphore_signal_full(struct semaphore_t* sem) {
	sem->rem = sem->cap;
}
