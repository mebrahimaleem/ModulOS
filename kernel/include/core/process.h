/* process.h - kernel process interface */
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

#ifndef KERNEL_CORE_PROCESS_H
#define KERNEL_CORE_PROCESS_H

#include <stdint.h>

#include <kernel/core/exception_dispatch.h>

struct pcb_t {
	// order is important
	uint64_t rsp; //0x00
	uint64_t rbp; //0x08
	uint64_t r15; //0x10
	uint64_t r14; //0x18
	uint64_t r13; //0x20
	uint64_t r12; //0x28
	uint64_t r11; //0x30
	uint64_t r10; //0x38
	uint64_t r9;  //0x40
	uint64_t r8;  //0x48
	uint64_t rdi; //0x50
	uint64_t rsi; //0x58
	uint64_t rdx; //0x60
	uint64_t rcx; //0x68
	uint64_t rbx; //0x70
	uint64_t rax; //0x78

	uint64_t rip; //0x80
	uint64_t cs;  //0x88
	uint64_t rflags; //0x90
	uint64_t ss;  // 0x98
	
	// end of important order

	uint64_t pid;
	uint32_t k_rsp_lo;
	uint32_t k_rsp_hi;
	uint64_t init_rsp;
	uint64_t init_k_rsp;
	enum {
		SCHED_READY,
		SCHED_KILL,
		SCHED_SKIP
	} sched_cntr;

	struct pcb_t* next;
};

struct preempt_frame_t {
	uint64_t rbp;
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t r11;
	uint64_t r10;
	uint64_t r9;
	uint64_t r8;
	uint64_t rdi;
	uint64_t rsi;
	uint64_t rdx;
	uint64_t rcx;
	uint64_t rbx;
	uint64_t rax;

	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;
} __attribute__((packed));

typedef void (*process_function_t)(void* cntx);

extern uint64_t process_get_pid(void);

extern void process_init(uint64_t init_rsp);

extern void process_init_ap(uint64_t init_rsp);

extern struct pcb_t* process_from_vaddr(uint64_t vaddr);

extern struct pcb_t* process_from_func(process_function_t func, void* cntx);

extern void process_resume(struct pcb_t* pcb) __attribute__((noreturn));

extern void process_kill_current(void) __attribute__((noreturn));

extern void process_discard(struct pcb_t* pcb);

extern void process_preempt_entry(struct preempt_frame_t* context) __attribute__((noreturn));

#endif /* KERNEL_CORE_PROCESS_H */
