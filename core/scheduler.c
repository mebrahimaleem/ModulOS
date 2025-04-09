/* scheduler.c - kernel scheduler */
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

#ifndef CORE_SCHEDULER_C
#define CORE_SCHEDULER_C

#include <stdint.h>

#include <core/atomic.h>
#include <core/memory.h>
#include <core/scheduler.h>

struct PCB* bot = 0;
struct PCB* top = 0;

mutex_t scheduler_mutex;

void scheduler_init() {
	bot = 0;
	top = 0;
	scheduler_mutex = kcreateMutex();
}

__attribute__((noreturn)) void scheduler_nextTask() {
	//TODO: handle sleeping and priority 
	/* wait for something to go in the queue */
	struct PCB pcb;
	struct PCB* temp;
	while (1) {
		kacquireMutex(scheduler_mutex);
		
		if (top != 0) {
			/* dequeue from queue */
			pcb = *top;
			temp = top;
			top = top->prev;
			if (top == 0) {
				bot = 0;
			}

			kfree(temp);

			kreleaseMutex(scheduler_mutex);
			break;
		}

		kreleaseMutex(scheduler_mutex);
		while (top == 0);
	}

	scheduler_transferTo(&pcb);
}

void scheduler_schedulePCB(struct PCB* pcb) {
	kacquireMutex(scheduler_mutex);

	/* enqueue to queue */
	pcb->next = bot;
	if (bot == 0) {
		pcb->prev = 0;
		top = pcb;
	}
	else {
		bot->prev = pcb;
	}
	bot = pcb;

	kreleaseMutex(scheduler_mutex);
}

#endif /* CORE_SCHEDULER_C */

