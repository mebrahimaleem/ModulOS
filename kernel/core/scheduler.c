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
#include <core/time.h>
#include <core/logging.h>

#include <apic/apic_regs.h>

static uint8_t lock_sched;
static struct pcb_t* active_queue;
static struct pcb_t* active_queue_tail;

static struct pcb_t* sleep_queue;

static void _scheduler_schedule(struct pcb_t* pcb) {
	pcb->next = 0;

	if (active_queue_tail) {
		active_queue_tail->next = pcb;
		active_queue_tail = active_queue_tail->next;
	}
	else {
		active_queue = pcb;
		active_queue_tail = pcb;
	}
}

void scheduler_schedule(struct pcb_t* pcb) {
	lock_acquire(&lock_sched);
	_scheduler_schedule(pcb);
	lock_release(&lock_sched);
}

void scheduler_init(void) {
	lock_init(&lock_sched);

	active_queue = 0;
	active_queue_tail = 0;
	sleep_queue = 0;
}

void scheduler_run(void) {
	struct proc_data_t* pd = proc_data_get();
	struct pcb_t* current_pcb = pd->current_process;
	if (current_pcb) {
		switch (current_pcb->sched_cntr) {
			case SCHED_SKIP:
				cpu_cli();
				apic_write_reg(APIC_REG_EOI, APIC_EOI);
				process_resume(current_pcb);
			case SCHED_KILL:
				process_discard(current_pcb);
				break;
			case SCHED_SLEEP:
				current_pcb->sched_cntr = SCHED_READY;

				lock_acquire(&lock_sched);
				struct pcb_t* i = sleep_queue, **prev = &sleep_queue;

				for (; i && current_pcb->sleep_state.wake_time < i->sleep_state.wake_time; i = i->next) {
					prev = &i->next;
				}
				
				*prev = current_pcb;
				current_pcb->next = i;

				lock_release(&lock_sched);
				break;
			case SCHED_CALLBACK:
				current_pcb->sleep_state.callback(current_pcb);
				break;
			case SCHED_READY:
			case SCHED_SIGNAL_READY:
				scheduler_schedule(current_pcb);
				break;
		}
	}

	lock_acquire(&lock_sched);

	// wakup sleeping processes
	const uint64_t now = time_since_init_fs();
	struct pcb_t* i, * next;
	for (i = sleep_queue; i && i->sleep_state.wake_time <= now; i = next) {
		next = i->next;
		_scheduler_schedule(i);
	}

	sleep_queue = i;

	struct pcb_t* run;
	if (active_queue) {
		// next process
		run = (struct pcb_t*)active_queue;
		active_queue = active_queue->next;
		if (!active_queue) {
			active_queue_tail = 0;
		}

		lock_release(&lock_sched);
	}
	else {
		// wait for process

		lock_release(&lock_sched);

		pd->current_process = 0;

		apic_write_reg(APIC_REG_EOI, APIC_EOI);
		cpu_wait_loop();
	}
	
	cpu_cli();

	pd->tss->rsp0_lo = run->k_rsp_lo;
	pd->tss->rsp0_hi = run->k_rsp_hi;
	pd->kernel_rsp = (uint64_t)run->k_rsp_lo | ((uint64_t)run->k_rsp_hi << 32);
	pd->current_process = run;
	cpu_set_cr3(run->cr3);
	cpu_set_fsbase(run->fsbase);
	cpu_restore_fx(run->fxdata);

	apic_write_reg(APIC_REG_EOI, APIC_EOI);

	process_resume(run);
}
