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
#include <core/cpulowlevel.h>
#include <core/memory.h>
#include <core/threads.h>
#include <core/scheduler.h>
#include <core/serial.h>

#include <apic/lapic.h>

#define QUANTUM_US	50000

struct PCB* volatile bot = 0;
struct PCB* volatile top = 0;

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
	struct PCB* tmp;
	while (1) {
		kacquireMutex(scheduler_mutex);


		if (top != 0) {
			/* dequeue from queue */
			pcb = *top;
			tmp = top;
			top = top->prev;
			if (top == 0) {
				bot = 0;
			}

			/* check state */
			switch (pcb.state) {
				case KILL:
					/* kill by freeing, and moving to next PCB */
					kfree((void*)pcb.fs); /* TLS */
					kfree(tmp); /* PCB */
					kreleaseMutex(scheduler_mutex);
					continue;
				case SLEEP:
					/* reschedule and try next */
					kreleaseMutex(scheduler_mutex);
					scheduler_schedulePCB(tmp);
					continue;
				default:
					break;
			}

			kreleaseMutex(scheduler_mutex);
			break;
		}

		kreleaseMutex(scheduler_mutex);
		while (top == 0)
			pause();
	}

	lapic_percpu[apic_getId()]->last_tls = pcb.tls;
	apic_setTimerDeadline(QUANTUM_US);
	scheduler_transferTo(&pcb);
}

__attribute__((noreturn)) void scheduler_reenter(struct PCB* pcb) {
	apic_lapic_sendeoi();
	ksti();

	struct PCB* oldpcb = lapic_percpu[apic_getId()]->last_tls->pcb;
	
	oldpcb->rip = pcb->rip;
	oldpcb->rfl = pcb->rfl;
	oldpcb->cr3 = pcb->cr3;
	oldpcb->cs  = pcb->cs;
	oldpcb->fs  = pcb->fs;
	oldpcb->ds  = pcb->ds;
	oldpcb->r15 = pcb->r15;
	oldpcb->r14 = pcb->r14;
	oldpcb->r13 = pcb->r13;
	oldpcb->r12 = pcb->r12;
	oldpcb->r11 = pcb->r11;
	oldpcb->r10 = pcb->r10;
	oldpcb->r9  = pcb->r9;
	oldpcb->r8  = pcb->r8;
	oldpcb->rdi = pcb->rdi;
	oldpcb->rsi = pcb->rsi;
	oldpcb->rbp = pcb->rbp;
	oldpcb->rsp = pcb->rsp;
	oldpcb->rdx = pcb->rdx;
	oldpcb->rcx = pcb->rcx;
	oldpcb->rbx = pcb->rbx;
	oldpcb->rax = pcb->rax;
	
	scheduler_schedulePCB(oldpcb);
	
	scheduler_nextTask();
}

void scheduler_schedulePCB(struct PCB* pcb) {
	kacquireMutex(scheduler_mutex);

	/* enqueue to queue */
	pcb->prev = 0;
	pcb->next = bot;
	if (bot == 0) {
		top = pcb;
	}
	else {
		bot->prev = pcb;
	}
	bot = pcb;

	kreleaseMutex(scheduler_mutex);
}

void scheduler_schedulePID(pid_t PID) {
	scheduler_schedulePCB(thread_PIDtoPCB(PID));
}

#endif /* CORE_SCHEDULER_C */

