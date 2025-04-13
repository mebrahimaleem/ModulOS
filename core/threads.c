/* threads.c - multithreading routines */
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

#ifndef CORE_THREADS_C
#define CORE_THREADS_C

#include <stdint.h>

#include <core/cpulowlevel.h>
#include <core/atomic.h>
#include <core/memory.h>
#include <core/scheduler.h>
#include <core/threads.h>

#include <apic/lapic.h>
#include <apic/pit.h>

#define KERNEL_STACK_SIZE	0x2000
#define KERNEL_DS					0x10
#define KERNEL_CS					0x08
#define KERNEL_FLAGS			0x200

#define INIT_THREADS			0x10

#define USED	1
#define FREE	0

#define FS_MSR	0xC0000100

#define STS_SLEEP	0
#define	STS_WAKE	1

pid_t maxPID;
struct Thread* threads;

mutex_t threadLock;

void thread_init() {
	maxPID = INIT_THREADS;
	threadLock = kcreateMutex();
	threads = kzalloc(sizeof(struct Thread) * INIT_THREADS);
}

pid_t thread_claimPID() {
	kacquireMutex(threadLock);
	for (uint64_t i = 0; i < maxPID; i++) {
		if (threads[i].used == FREE) {
				threads[i].used = USED;
				kreleaseMutex(threadLock);
				return i;
		}
	}
	/* otherwise allocate more threads */
	const pid_t ret = maxPID;
	maxPID *= 2;
	threads = kzrealloc(threads, sizeof(struct Thread) * maxPID);
	threads[ret].used = USED;
	kreleaseMutex(threadLock);
	return ret;
}

pid_t thread_kernelFromAddress(uint64_t addr) {
	struct TLS* tls = kmalloc(sizeof(struct TLS));

	struct PCB* ret = kmalloc(sizeof(struct PCB));
	ret->rax =
		ret->rbx =
		ret->rcx =
		ret->rdx =
		ret->rbp =
		ret->rsi =
		ret->rdi =
		ret->r8 =
		ret->r9 =
		ret->r10 =
		ret->r11 =
		ret->r12 =
		ret->r13 =
		ret->r14 =
		ret->r15 = 0;

	ret->rsp = (uint64_t)kmalloc(KERNEL_STACK_SIZE) + KERNEL_STACK_SIZE;

	ret->ds = KERNEL_DS;
	ret->cs = KERNEL_CS;
	ret->cr3 = (uint64_t)kPML4T;

	ret->rfl = KERNEL_FLAGS;
	ret->rip = addr;

	ret->state = RUNNING;
	ret->tls = tls;

	tls->pcb = ret;
	tls->PID = thread_claimPID();
	threads[tls->PID].tls = tls;

	return tls->PID;
}

struct PCB* thread_PIDtoPCB(pid_t PID) {
	return threads[PID].tls->pcb;
}

void thread_kill(pid_t PID) {
	kacquireMutex(threadLock);
	threads[PID].tls->pcb->state = KILL;
	threads[PID].used = FREE;
	kreleaseMutex(threadLock);
}

pid_t thread_getPID() {
	return lapic_percpu[apic_getId()]->last_tls->PID;
}

void thread_wait(uint64_t time) {
	const pid_t PID = thread_getPID();
	threads[PID].wakeSignal = STS_SLEEP;
	apic_addDeadline(PID, time);
	thread_enterSleep();
}

void thread_enterSleep() {
	const pid_t PID = thread_getPID();
	threads[PID].tls->pcb->state = SLEEP;

	while (threads[PID].wakeSignal == STS_SLEEP)
		pause();
}

void thread_wakeup(pid_t PID) {
	threads[PID].wakeSignal = STS_WAKE;
	threads[PID].tls->pcb->state = RUNNING;
}

#endif /* CORE_THREADS_C */

