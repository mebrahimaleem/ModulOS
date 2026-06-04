/* signal.c - kernel signal implementation */
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

#include <core/signal.h>
#include <core/lock.h>
#include <core/process.h>
#include <core/scheduler.h>
#include <core/alloc.h>
#include <core/proc_data.h>
#include <core/cpu_instr.h>

struct signal_wait_t {
	struct pcb_t* queue;
	uint8_t lock;
};

static void signal_wait_callback(struct pcb_t* pcb) {
	struct signal_wait_t* wait = pcb->meta[0];
	uint8_t* rlock = pcb->meta[1];

	lock_acquire(&wait->lock);
	pcb->next = wait->queue;
	wait->queue = pcb;
	lock_release(&wait->lock);

	if (rlock) {
		lock_release(rlock);
	}
}

struct signal_wait_t* signal_wait_alloc(void) {
	struct signal_wait_t* ret = kmalloc(sizeof(struct signal_wait_t));

	lock_init(&ret->lock);
	ret->queue = 0;

	return ret;
}

void signal_wait(struct signal_wait_t* wait) {
	signal_wait_locked(wait, 0);
}

void signal_awake(struct signal_wait_t* wait) {
	signal_awake_locked(wait, 0);
}

void signal_wait_locked(struct signal_wait_t* wait, uint8_t* rlock) {
	struct pcb_t* current = proc_data_get()->current_process;

	current->meta[0] = wait;
	current->meta[1] = rlock;
	process_set_callback(signal_wait_callback);

	while (current->sched_cntr != SCHED_SIGNAL_READY) {
		cpu_hlt();
	}

	current->sched_cntr = SCHED_READY;
}

void signal_awake_locked(struct signal_wait_t* wait, uint8_t* rlock) {
	struct pcb_t* i, * next;
	lock_acquire(&wait->lock);
	i = wait->queue;
	wait->queue = 0;
	lock_release(&wait->lock);
	if (rlock) {
		lock_release(rlock);
	}

	for (; i; i = next) {
		next = i->next;

		i->sched_cntr = SCHED_SIGNAL_READY;
		scheduler_schedule(i);
	}
}


void signal_free(struct signal_wait_t* wait) {
	signal_awake(wait);
	kfree(wait);
}
