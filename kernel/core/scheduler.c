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
	process_set_next(pcb, 0);

	if (active_queue_tail) {
		process_set_next(active_queue_tail, pcb);
		active_queue_tail = pcb;
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
		switch (process_get_sched_cntr(current_pcb)) {
			case SCHED_SKIP:
				cpu_cli();
				apic_write_reg(APIC_REG_EOI, APIC_EOI);
				process_resume(current_pcb);
			case SCHED_KILL:
				process_discard(current_pcb);
				break;
			case SCHED_SLEEP:
				process_set_sched_cntr(current_pcb, SCHED_READY);

				lock_acquire(&lock_sched);
				struct pcb_t* i = sleep_queue, **prev = &sleep_queue;

				for (; i && process_get_wake_time(current_pcb) < process_get_wake_time(i); i = process_get_next(i)) {
					prev = process_next_ref(i);
				}
				
				*prev = current_pcb;
				process_set_next(current_pcb, i);

				lock_release(&lock_sched);
				break;
			case SCHED_CALLBACK:
				process_call_callback(current_pcb);
				break;
			case SCHED_READY:
			case SCHED_SIGNAL_READY:
				scheduler_schedule(current_pcb);
				break;
			case SCHED_ZOMBIE:
				logging_log_warning("Zombie process still running");
				break;
		}
	}

	lock_acquire(&lock_sched);

	// wakup sleeping processes
	const uint64_t now = time_since_init_fs();
	struct pcb_t* i, * next;
	for (i = sleep_queue; i && process_get_wake_time(i) <= now; i = next) {
		next = process_get_next(i);
		_scheduler_schedule(i);
	}

	sleep_queue = i;

	struct pcb_t* run;
	if (active_queue) {
		// next process
		run = (struct pcb_t*)active_queue;
		active_queue = process_get_next(active_queue);
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
	
	process_resume_transfer(run);
}
