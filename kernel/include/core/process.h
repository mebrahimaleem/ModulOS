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
#include <stddef.h>

#include <kernel/core/exception_dispatch.h>
#include <kernel/core/fs.h>
#include <kernel/core/signal.h>

#include <kernel/lib/array_list.h>
#include <kernel/lib/hash_table.h>

#define MAX_META		2

struct pcb_t;

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

enum sched_cntr_t {
	SCHED_READY,
	SCHED_KILL,
	SCHED_SKIP,
	SCHED_SLEEP,
	SCHED_CALLBACK,
	SCHED_SIGNAL_READY,
	SCHED_ZOMBIE
};

typedef void (*process_function_t)(void* cntx);

extern uint64_t process_get_pid(void);

extern uint64_t process_assign_pid(void);

extern void process_init(uint64_t init_rsp_vaddr, uint64_t init_rsp_paddr);

extern void process_init_ap(uint64_t init_rsp_vaddr, uint64_t init_rsp_paddr);

extern struct pcb_t* process_from_vaddr(uint64_t vaddr);

extern struct pcb_t* process_from_func(process_function_t func, void* cntx);

extern void process_resume(struct pcb_t* pcb) __attribute__((noreturn));

extern void process_kill_current(void) __attribute__((noreturn));

extern void process_discard(struct pcb_t* pcb);

extern void process_preempt_entry(struct preempt_frame_t* context) __attribute__((noreturn));

extern uint8_t process_create_guarded_stack(uint64_t* init_vaddr, uint64_t* init_paddr);

extern void process_sleep(uint64_t wake_time);

extern void process_set_callback(void (*callback)(struct pcb_t*));

extern uint64_t process_fork(uint64_t r11, uint64_t rcx, uint64_t rbp);

extern void process_init_reaper(void);

extern uint64_t process_reap_child(struct pcb_t* pcb);

extern uint64_t process_find_stack_top(uint64_t vaddr_base);

extern uint8_t process_is_userland(void);

extern uint64_t process_register_fd(struct fs_handle_t* handle);

extern struct fs_handle_t* process_resolve_fd(uint64_t fd);

extern void process_remove_fd(uint64_t fd);

extern struct fs_handle_t* process_replace_fd(uint64_t fd, struct fs_handle_t* handle);

extern void process_exit(uint64_t ec) __attribute__((noreturn));

extern struct fs_handle_t* process_get_wd();

extern void process_set_wd(struct fs_handle_t* handle);

extern uint64_t process_get_mem_top(void);

extern void process_set_mem_top(uint64_t top);

extern uint64_t process_get_cr3(void);

extern void process_set_cr3(uint64_t cr3);

extern uint64_t process_get_ppid(void);

extern uint64_t process_wait_pid(uint64_t pid, uint8_t no_hang, uint64_t* ec_ret);

extern void** process_get_meta(struct pcb_t* pcb, size_t num);

extern void process_execve(struct pcb_t* actual, struct pcb_t* desired);

extern enum sched_cntr_t process_get_sched_cntr(struct pcb_t* pcb);

extern void process_set_sched_cntr(struct pcb_t* pcb, enum sched_cntr_t cntr);

extern void process_set_next(struct pcb_t* pcb, struct pcb_t* next);

extern struct pcb_t* process_get_next(struct pcb_t* pcb);

extern struct pcb_t** process_next_ref(struct pcb_t* pcb);

extern uint64_t process_get_wake_time(struct pcb_t* pcb);

extern void process_call_callback(struct pcb_t* pcb);

extern void process_resume_transfer(struct pcb_t* run) __attribute__((noreturn));

extern struct pcb_t* process_create_userland_pcb(uint64_t rdi,
																								 uint64_t rdx,
																								 uint64_t stack_vaddr,
																								 uint64_t stack_paddr,
																								 uint64_t cr3,
																								 struct pcb_t* parent,
																								 uint64_t memtop,
																								 uint64_t pid);

#endif /* KERNEL_CORE_PROCESS_H */
