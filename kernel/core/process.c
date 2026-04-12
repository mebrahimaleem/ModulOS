/* process.c - kernel process manager */
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

#include <core/process.h>
#include <core/alloc.h>
#include <core/lock.h>
#include <core/panic.h>
#include <core/logging.h>
#include <core/proc_data.h>
#include <core/cpu_instr.h>
#include <core/gdt.h>
#include <core/scheduler.h>
#include <core/paging.h>
#include <core/mm.h>

#include <lib/kmemset.h>

#include <apic/apic_regs.h>

#define INIT_RFLG				0x200
#define INIT_STACK_SIZE 0x4000

// change manual paging map when adjusting stack size

static uint64_t next_pid;
static uint8_t lock_proc;

__attribute__((noreturn)) static void function_setup(process_function_t func, void* cntx) {
	func(cntx);
	process_kill_current();
}

void process_init(uint64_t init_rsp_vaddr, uint64_t init_rsp_paddr) {
	next_pid = 1;
	lock_init(&lock_proc);
	process_init_ap(init_rsp_vaddr, init_rsp_paddr);
}

uint64_t process_assign_pid(void) {
	lock_acquire(&lock_proc);
	const uint64_t ret = next_pid++;
	lock_release(&lock_proc);
	return ret;
}

uint64_t process_get_pid(void) {
	return proc_data_get()->current_process->pid;
}

void process_init_ap(uint64_t init_rsp_vaddr, uint64_t init_rsp_paddr) {
	struct pcb_t* pcb = kmalloc(sizeof(struct pcb_t));
	pcb->init_k_rsp_vaddr = init_rsp_vaddr;
	pcb->init_k_rsp_paddr = init_rsp_paddr;
	pcb->sched_cntr = SCHED_SKIP;
	proc_data_get()->current_process = pcb;
	proc_data_get()->current_process->pid = process_assign_pid();
	proc_data_get()->current_process->cr3 = 0;
}

struct pcb_t* process_from_vaddr(uint64_t vaddr) {
	uint64_t stack_paddr, stack_vaddr, rsp;
	struct pcb_t* pcb;

	if (process_create_guarded_stack(&stack_vaddr, &stack_paddr, &rsp)) {
		logging_log_error("Failed to allocate stack");
		return 0;
	}

	pcb = kmalloc(sizeof(struct pcb_t));

	pcb->rax =
		pcb->rbx =
		pcb->rcx =
		pcb->rdx =
		pcb->rbp =
		pcb->rsi =
		pcb->rdi =
		pcb->r8 =
		pcb->r9 =
		pcb->r10 =
		pcb->r11 =
		pcb->r12 =
		pcb->r13 =
		pcb->r14 =
		pcb->r15 = 0;

	pcb->rsp = rsp;
	pcb->init_k_rsp_vaddr = stack_vaddr;
	pcb->init_k_rsp_paddr = stack_paddr;

	pcb->saved_usr_rsp = 0;
	pcb->k_rsp_lo = 0;
	pcb->k_rsp_hi = 0;

	pcb->rflags = INIT_RFLG;

	pcb->rip = vaddr;
	pcb->cs = GDT_KERNEL_CS;
	pcb->ss = GDT_KERNEL_SS;

	pcb->sched_cntr = SCHED_READY;

	pcb->cr3 = 0;

	pcb->pid = process_assign_pid();

	return pcb;
}

struct pcb_t* process_from_func(process_function_t func, void* cntx) {
	struct pcb_t* pcb = process_from_vaddr((uint64_t)function_setup);

	if (!pcb) {
		return 0;
	}

	pcb->rdi = (uint64_t)func;
	pcb->rsi = (uint64_t)cntx;

	return pcb;
}

void process_kill_current(void) {
	lock_acquire(&lock_proc);
	struct pcb_t* pcb = proc_data_get()->current_process;
	pcb->sched_cntr = SCHED_KILL;
	lock_release(&lock_proc);

	cpu_wait_loop();
}

void process_discard(struct pcb_t* pcb) {
	_Static_assert(INIT_STACK_SIZE == 4 * PAGE_SIZE_4K, "stack size must be page size multiple of four");
	paging_unmap(pcb->init_k_rsp_vaddr + 1 * PAGE_SIZE_4K, PAGE_4K);
	paging_unmap(pcb->init_k_rsp_vaddr + 2 * PAGE_SIZE_4K, PAGE_4K);
	paging_unmap(pcb->init_k_rsp_vaddr + 3 * PAGE_SIZE_4K, PAGE_4K);
	paging_unmap(pcb->init_k_rsp_vaddr + 4 * PAGE_SIZE_4K, PAGE_4K);
	paging_remove_guard(pcb->init_k_rsp_vaddr);

	mm_free_v(pcb->init_k_rsp_vaddr, INIT_STACK_SIZE + PAGE_SIZE_4K);
	mm_free_p(pcb->init_k_rsp_paddr, INIT_STACK_SIZE);

	if (pcb->cr3) {
		paging_free_userspace((uint64_t*)pcb->cr3);
	}

	logging_log_debug("Killed %ld", pcb->pid);

	kfree(pcb);
}

void process_preempt_entry(struct preempt_frame_t* context) {
	struct pcb_t* pcb = proc_data_get()->current_process;
	pcb->rsp = context->rsp;
	pcb->rbp = context->rbp;
	pcb->r15 = context->r15;
	pcb->r14 = context->r14;
	pcb->r13 = context->r13;
	pcb->r12 = context->r12;
	pcb->r11 = context->r11;
	pcb->r10 = context->r10;
	pcb->r9 = context->r9;
	pcb->r8 = context->r8;
	pcb->rdi = context->rdi;
	pcb->rsi = context->rsi;
	pcb->rdx = context->rdx;
	pcb->rcx = context->rcx;
	pcb->rbx = context->rbx;
	pcb->rax = context->rax;
	pcb->rip = context->rip;
	pcb->cs = context->cs;
	pcb->rflags = context->rflags;
	pcb->ss = context->ss;

	scheduler_run();
}

uint8_t process_create_guarded_stack(uint64_t* init_vaddr, uint64_t* init_paddr, uint64_t* stack) {
	uint64_t stack_vaddr;
	uint64_t stack_paddr;

	stack_vaddr = mm_alloc_v(INIT_STACK_SIZE + PAGE_SIZE_4K); // extra guard page
	if (!stack_vaddr) {
		return 1;
	}

	stack_paddr = mm_alloc_p(INIT_STACK_SIZE);
	if (!stack_paddr) {
		mm_free_v(stack_vaddr, INIT_STACK_SIZE + PAGE_SIZE_4K);
		return 1;
	}

	_Static_assert(INIT_STACK_SIZE == 4 * PAGE_SIZE_4K, "stack size must be four pages (16KiB)");
	// add for increased stack size
	paging_map(stack_vaddr + 1 * PAGE_SIZE_4K, stack_paddr + 0 * PAGE_SIZE_4K, PAGE_PRESENT | PAGE_RW, PAGE_4K);
	paging_map(stack_vaddr + 2 * PAGE_SIZE_4K, stack_paddr + 1 * PAGE_SIZE_4K, PAGE_PRESENT | PAGE_RW, PAGE_4K);
	paging_map(stack_vaddr + 3 * PAGE_SIZE_4K, stack_paddr + 2 * PAGE_SIZE_4K, PAGE_PRESENT | PAGE_RW, PAGE_4K);
	paging_map(stack_vaddr + 4 * PAGE_SIZE_4K, stack_paddr + 3 * PAGE_SIZE_4K, PAGE_PRESENT | PAGE_RW, PAGE_4K);
	// leave last page unmapped as guard
	paging_install_guard(stack_vaddr);

	kmemset((uint8_t*)stack_vaddr + 1 * PAGE_SIZE_4K, 0, 4 * PAGE_SIZE_4K);

	*init_vaddr = stack_vaddr;
	*init_paddr = stack_paddr;
	*stack = stack_vaddr + PAGE_SIZE_4K * 5;

	return 0;
}
