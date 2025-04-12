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

#include <core/memory.h>
#include <core/scheduler.h>
#include <core/threads.h>

#define KERNEL_STACK_SIZE	0x2000
#define KERNEL_DS					0x10
#define KERNEL_CS					0x08
#define KERNEL_FLAGS			0x200

uint64_t nextPID = 0;

uint64_t thread_generatePID() {
	return nextPID++; //TODO: handle wraparound
}

struct PCB* thread_kernelFromAddress(uint64_t addr) {
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
	ret->fs = (uint64_t)tls;
	ret->cs = KERNEL_CS;
	ret->cr3 = (uint64_t)kPML4T;

	ret->rfl = KERNEL_FLAGS;
	ret->rip = addr;

	ret->state = RUNNING;

	tls->pcb = ret;
	tls->PID = thread_generatePID();
	return ret;
}

#endif /* CORE_THREADS_C */

