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

			kfree((struct PCB*)temp);

			kreleaseMutex(scheduler_mutex);
			break;
		}

		kreleaseMutex(scheduler_mutex);
		while (top == 0)
			pause();
	}

	apic_setTimerDeadline(QUANTUM_US);
	scheduler_transferTo(&pcb);
}

__attribute__((noreturn)) void scheduler_reenter(struct PCB* pcb) {
	apic_lapic_sendeoi();

	struct PCB* nonvolatile = kmalloc(sizeof(struct PCB));
	
	nonvolatile->rip = pcb->rip;
	nonvolatile->rfl = pcb->rfl;
	nonvolatile->cr3 = pcb->cr3;
	nonvolatile->cs  = pcb->cs;
	nonvolatile->fs  = pcb->fs;
	nonvolatile->ds  = pcb->ds;
	nonvolatile->r15 = pcb->r15;
	nonvolatile->r14 = pcb->r14;
	nonvolatile->r13 = pcb->r13;
	nonvolatile->r12 = pcb->r12;
	nonvolatile->r11 = pcb->r11;
	nonvolatile->r10 = pcb->r10;
	nonvolatile->r9  = pcb->r9;
	nonvolatile->r8  = pcb->r8;
	nonvolatile->rdi = pcb->rdi;
	nonvolatile->rsi = pcb->rsi;
	nonvolatile->rbp = pcb->rbp;
	nonvolatile->rsp = pcb->rsp;
	nonvolatile->rdx = pcb->rdx;
	nonvolatile->rcx = pcb->rcx;
	nonvolatile->rbx = pcb->rbx;
	nonvolatile->rax = pcb->rax;
	
	scheduler_schedulePCB(nonvolatile);
	
	scheduler_nextTask();
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

