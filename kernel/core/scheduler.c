/* scheduler.c - kernel scheduler */
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

#include <core/scheduler.h>
#include <core/process.h>
#include <core/lock.h>
#include <core/cpu_instr.h>
#include <core/alloc.h>
#include <core/proc_data.h>
#include <core/gdt.h>

static uint8_t lock_sched;
static volatile struct pcb_t* active_queue;
static volatile struct pcb_t* active_queue_tail;

void scheduler_schedule(struct pcb_t* pcb) {
	pcb->next = 0;
	lock_acquire(&lock_sched);
	if (active_queue_tail) {
		active_queue_tail->next = pcb;
	}
	else {
		active_queue = pcb;
		active_queue_tail = pcb;
	}
	lock_release(&lock_sched);
}

void scheduler_init(void) {
	lock_init(&lock_sched);

	active_queue = 0;
	active_queue_tail = 0;
}

void scheduler_run(void) {
	struct proc_data_t* pd = proc_data_get();
	struct pcb_t* current_pcb = pd->current_process;
	switch (current_pcb->sched_cntr) {
		case SCHED_SKIP:
			process_resume(current_pcb);
			break;
		case SCHED_KILL:
			kfree((void*)current_pcb->init_rsp);
			kfree((void*)current_pcb->init_k_rsp);
			break;
		default:
			scheduler_schedule(current_pcb);
			break;
	}

	struct pcb_t* run = 0;
	while (!run) {
		cpu_pause();
		lock_acquire(&lock_sched);
		if (active_queue) {
			run = (struct pcb_t*)active_queue;
			active_queue = active_queue->next;
			if (!active_queue) {
				active_queue_tail = 0;
			}
			lock_release(&lock_sched);
			break;
		}
		lock_release(&lock_sched);
	}

	pd->tss->rsp0_lo = 	run->k_rsp_lo;
	pd->tss->rsp0_hi = 	run->k_rsp_hi;
	pd->current_process = run;
	process_resume(run);
}
