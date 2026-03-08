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

#include <drivers/apic/apic_regs.h>

#define INIT_RFLG				0x200
#define INIT_STACK_SIZE 0x4000

static uint64_t next_pid;
static uint8_t lock_proc;

__attribute__((noreturn)) static void function_setup(process_function_t func, void* cntx) {
	func(cntx);
	process_kill_current();
}

void process_init(uint64_t init_rsp) {
	next_pid = 1;
	lock_init(&lock_proc);
	process_init_ap(init_rsp);
}

static uint64_t assign_pid(void) {
	lock_acquire(&lock_proc);
	const uint64_t ret = next_pid++;
	lock_release(&lock_proc);
	return ret;
}

uint64_t process_get_pid(void) {
	return proc_data_get()->current_process->pid;
}

void process_init_ap(uint64_t init_rsp) {
	struct pcb_t* pcb = kmalloc(sizeof(struct pcb_t));
	pcb->init_rsp = init_rsp;
	pcb->init_k_rsp = 0;
	pcb->sched_cntr = SCHED_SKIP;
	proc_data_get()->current_process = pcb;
	proc_data_get()->current_process->pid = assign_pid();
}

struct pcb_t* process_from_vaddr(uint64_t vaddr) {
	struct pcb_t* pcb = kmalloc(sizeof(struct pcb_t));

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

	pcb->rsp =
		pcb->init_rsp = (uint64_t)kmalloc(INIT_STACK_SIZE);
	pcb->init_k_rsp = 0;
	pcb->rsp += INIT_STACK_SIZE;
	pcb->k_rsp_lo = 0;
	pcb->k_rsp_hi = 0;

	pcb->rflags = INIT_RFLG;

	pcb->rip = vaddr;
	pcb->cs = GDT_KERNEL_CS;
	pcb->ss = GDT_KERNEL_SS;

	pcb->sched_cntr = SCHED_READY;

	pcb->pid = assign_pid();

	return pcb;
}

struct pcb_t* process_from_func(process_function_t func, void* cntx) {
	struct pcb_t* pcb = process_from_vaddr((uint64_t)function_setup);

	pcb->rdi = (uint64_t)func;
	pcb->rsi = (uint64_t)cntx;

	return pcb;
}

void process_kill_current(void) {
	cpu_cli();
	lock_acquire(&lock_proc);
	struct pcb_t* pcb = proc_data_get()->current_process;
	pcb->sched_cntr = SCHED_KILL;
	logging_log_debug("Killed %ld", pcb->pid);
	lock_release(&lock_proc);
	cpu_sti();

	cpu_wait_loop();
}

void process_discard(struct pcb_t* pcb) {
	kfree((void*)pcb->init_rsp);
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

	apic_write_reg(APIC_REG_EOI, APIC_EOI);

	scheduler_run();
}
