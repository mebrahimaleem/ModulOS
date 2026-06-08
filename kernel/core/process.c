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
#include <core/time.h>
#include <core/fs.h>
#include <core/syscall.h>
#include <core/signal.h>
#include <core/panic.h>

#include <lib/kmemset.h>
#include <lib/kmemcpy.h>
#include <lib/array_list.h>
#include <lib/hash_table.h>

#include <apic/apic_regs.h>

#define INIT_RFLG				0x200
#define INIT_STACK_SIZE 0x4000
#define REAP_DELAY_MS		1000

#define FD_INIT_SIZE		4
#define FD_GROWTH				8
#define CHILD_BUCKETS		4

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
	uint64_t init_k_rsp_vaddr;
	uint64_t init_k_rsp_paddr;
	uint64_t fsbase;
	uint64_t mem_top;

	uint64_t cr3;

	struct pcb_t* next;

	uint64_t exit_code;

	struct fs_handle_t* wd;
	struct array_list_t* fd_table;

	uint8_t fxdata[512] __attribute__((aligned(16)));

	struct hash_table_t* child_table;
	struct pcb_t* parent;
	struct signal_wait_t* monitor;

	void* meta[MAX_META];

	union {
		uint64_t wake_time;
		void (*callback)(struct pcb_t*);
	} sleep_state;

	enum sched_cntr_t sched_cntr;

	uint8_t plock;
};

static inline struct pcb_t* init_pcb(uint64_t _rip,
																		 struct fs_handle_t* _wd,
																		 struct array_list_t* _fd_table,
																		 struct hash_table_t* _child_table,
																		 struct pcb_t* _parent,
																		 struct signal_wait_t* _monitor,
								 										 uint64_t _cr3,
																		 uint64_t _pid,
																		 uint64_t _vaddr,
																		 uint64_t _paddr,
																		 uint64_t _mem_top) {
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
	pcb->rflags = INIT_RFLG;
	pcb->fsbase = 0;
	pcb->cs = GDT_KERNEL_CS;
	pcb->ss = GDT_KERNEL_SS;
	pcb->rip = _rip;
	lock_init(&pcb->plock);
	pcb->sched_cntr = SCHED_READY;
	pcb->wd = _wd;
	pcb->fd_table = _fd_table;
	pcb->child_table = _child_table;
	pcb->parent = _parent;
	cpu_save_fx(pcb->fxdata);
	pcb->monitor = _monitor;
	pcb->cr3 = _cr3;
	pcb->pid = _pid;
	pcb->init_k_rsp_vaddr = _vaddr;
	pcb->init_k_rsp_paddr = _paddr;
	pcb->rsp = process_find_stack_top(_vaddr);
	pcb->mem_top = _mem_top;

	pcb->k_rsp_lo = pcb->rsp & 0xFFFFFFFF;
	pcb->k_rsp_hi = pcb->rsp >> 32;

	return pcb;
}

static uint64_t next_pid;
static uint8_t lock_proc;

static uint8_t lock_reap;

static struct pcb_t* reap_queue;

static struct signal_wait_t* reap_wait;

static struct pcb_t* init_reaper_pcb;

__attribute__((noreturn)) static void function_setup(process_function_t func, void* cntx) {
	func(cntx);
	process_kill_current();
}

void process_init(uint64_t init_rsp_vaddr, uint64_t init_rsp_paddr) {
	next_pid = 2;
	reap_queue = 0;
	init_reaper_pcb = 0;
	lock_init(&lock_proc);
	lock_init(&lock_reap);
	reap_wait = signal_wait_alloc();
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
	struct pcb_t* pcb = init_pcb(0,
															 0,
															 0,
															 0,
															 0,
															 0,
															 0,
															 process_assign_pid(),
															 init_rsp_vaddr,
															 init_rsp_paddr,
															 0);

	proc_data_get()->current_process = pcb;
}

struct pcb_t* process_from_vaddr(uint64_t vaddr) {
	uint64_t stack_paddr, stack_vaddr;
	struct pcb_t* pcb;

	if (process_create_guarded_stack(&stack_vaddr, &stack_paddr)) {
		logging_log_error("Failed to allocate stack");
		return 0;
	}

	pcb = init_pcb(vaddr,
								 0,
								 0,
								 0,
								 0,
								 0,
								 0,
								 process_assign_pid(),
								 stack_vaddr,
								 stack_paddr,
								 0);
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
	pcb->exit_code = 0;
	pcb->sched_cntr = SCHED_KILL;
	lock_release(&lock_proc);

	cpu_wait_loop();
}

static void close_fd(void* handle) {
	fs_close(handle);
}

void process_discard(struct pcb_t* pcb) {
	if (pcb->pid == 1) {
		logging_log_error("Init process discarded");
		panic(PANIC_STATE);
	}

	lock_acquire(&lock_reap);
	pcb->next = reap_queue;
	reap_queue = pcb;
	lock_release(&lock_reap);

	signal_awake(reap_wait);
}

void process_preempt_entry(struct preempt_frame_t* context) {
	struct pcb_t* pcb = proc_data_get()->current_process;
	if (pcb) {
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
		pcb->fsbase = cpu_get_fsbase();
		cpu_save_fx(pcb->fxdata);
	}

	scheduler_run();
}

static void process_copy_stack(uint64_t src, uint64_t dest) {
	kmemcpy((void*)(dest + PAGE_SIZE_4K), (void*)(src + PAGE_SIZE_4K), INIT_STACK_SIZE);
}

uint8_t process_create_guarded_stack(uint64_t* init_vaddr, uint64_t* init_paddr) {
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

	return 0;
}

void process_sleep(uint64_t wake_time) {
	struct pcb_t* current = proc_data_get()->current_process;

	current->sleep_state.wake_time = wake_time;
	current->sched_cntr = SCHED_SLEEP;

	while (time_since_init_fs() < wake_time) {
		cpu_hlt();
	}
}

void process_set_callback(void (*callback)(struct pcb_t*)) {
	struct pcb_t* current = proc_data_get()->current_process;

	current->sleep_state.callback = callback;
	current->sched_cntr = SCHED_CALLBACK;
}

static void* fd_dup(void* fd) {
	return fs_dup(fd);
}

uint64_t process_fork(uint64_t r11, uint64_t rcx, uint64_t rbp) {
	uint64_t stack_paddr, stack_vaddr;

	if (process_create_guarded_stack(&stack_vaddr, &stack_paddr)) {
		logging_log_error("Failed to allocate stack");
		return 0;
	}

	struct pcb_t* parent = proc_data_get()->current_process;

	struct pcb_t* child = init_pcb((uint64_t)syscall_return,
																 fs_dup(parent->wd),
																 array_list_dup(parent->fd_table, fd_dup),
																 hash_table_alloc(CHILD_BUCKETS),
																 parent,
																 signal_wait_alloc(),
																 paging_duplicate_lower(parent->cr3),
																 process_assign_pid(),
																 stack_vaddr,
																 stack_paddr,
																 parent->mem_top);

	process_copy_stack(parent->init_k_rsp_vaddr, child->init_k_rsp_vaddr);

	child->rbp = *(uint64_t*)rbp;  // rbp (on userland stack) stores pointer to rbp

	child->rcx = 0;
	child->rsi = r11;
	child->rdi = rcx;
	child->rdx = rbp + 8;  // rsp address must be right above rbp (pushed on userland stack)

	scheduler_schedule(child);

	hash_table_insert(parent->child_table, child->pid, child);

	return child->pid;
}

static void reap_final(struct pcb_t* pcb);

static void orphan_children(void* child) {
	struct pcb_t* pcb = child;

	pcb->parent = init_reaper_pcb;
	hash_table_insert(init_reaper_pcb->child_table, pcb->pid, pcb);
}

static void reap_final(struct pcb_t* pcb) {
	void* ign;
	hash_table_remove(pcb->parent->child_table, pcb->pid, &ign);

	signal_free(pcb->monitor);

	kfree(pcb);
}

static void reap_prepare(struct pcb_t* pcb) {
	_Static_assert(INIT_STACK_SIZE == 4 * PAGE_SIZE_4K, "stack size must be page size multiple of four");

	uint8_t stale = 1;

	if (pcb->init_k_rsp_vaddr) {
		paging_unmap(pcb->init_k_rsp_vaddr + 1 * PAGE_SIZE_4K, PAGE_4K);
		paging_unmap(pcb->init_k_rsp_vaddr + 2 * PAGE_SIZE_4K, PAGE_4K);
		paging_unmap(pcb->init_k_rsp_vaddr + 3 * PAGE_SIZE_4K, PAGE_4K);
		paging_unmap(pcb->init_k_rsp_vaddr + 4 * PAGE_SIZE_4K, PAGE_4K);
		paging_remove_guard(pcb->init_k_rsp_vaddr);

		mm_free_p(pcb->init_k_rsp_paddr, INIT_STACK_SIZE);
		mm_free_v(pcb->init_k_rsp_vaddr, INIT_STACK_SIZE + PAGE_SIZE_4K);

		stale = 0;
	}

	if (pcb->cr3) {
		// userland pcb

		paging_free_userspace((uint64_t*)pcb->cr3);

		// execve creates stale pcbs that must not cleanup heiracrhy resources
		if (stale) {
			kfree(pcb);
		}
		else {
			fs_close(pcb->wd);
			array_list_free(pcb->fd_table, close_fd);

			// orphan all children
			hash_table_free(pcb->child_table, orphan_children);

			lock_acquire(&pcb->plock);
			pcb->sched_cntr = SCHED_ZOMBIE;
			signal_awake_locked(pcb->monitor, &pcb->plock);
		}
	}
}

__attribute__((noreturn)) static void process_reap(void* _ign) {
	(void)_ign;

	while (1) {
		time_sleep(REAP_DELAY_MS);

		while (!reap_queue) {
			signal_wait(reap_wait);
		}

		struct pcb_t* pcb, * next;

		lock_acquire(&lock_reap);
		pcb = reap_queue;
		reap_queue = 0;
		lock_release(&lock_reap);

		for (; pcb; pcb = next) {
			next = pcb->next;

			reap_prepare(pcb);
		}
	}
}

void process_init_reaper(void) {
	struct pcb_t* reaper = process_from_func(process_reap, 0);
	scheduler_schedule(reaper);
}

uint64_t process_reap_child(struct pcb_t* pcb) {
	while (1) {
		lock_acquire(&pcb->plock);

		if (pcb->sched_cntr == SCHED_ZOMBIE) {
			break;
		}
		signal_wait_locked(pcb->monitor, &pcb->plock);
	}
	lock_release(&pcb->plock);

	uint64_t ret = pcb->exit_code;

	reap_final(pcb);

	return ret;
}

uint64_t process_find_stack_top(uint64_t vaddr_base) {
	_Static_assert(INIT_STACK_SIZE == 4 * PAGE_SIZE_4K, "stack size must be four pages (16KiB)");
	return vaddr_base + PAGE_SIZE_4K * 5;
}

uint8_t process_is_userland(void) {
	// only userland tasks have cr3s
	return !!proc_data_get()->current_process->cr3;
}

struct fs_handle_t* process_resolve_fd(uint64_t fd) {
	struct pcb_t* pcb = proc_data_get()->current_process;
	return array_list_get(pcb->fd_table, fd);
}

uint64_t process_register_fd(struct fs_handle_t* handle) {
	struct pcb_t* pcb = proc_data_get()->current_process;
	return array_list_push(pcb->fd_table, handle);
}

void process_remove_fd(uint64_t fd) {
	struct pcb_t* pcb = proc_data_get()->current_process;
	array_list_remove(pcb->fd_table, fd);
}

struct fs_handle_t* process_replace_fd(uint64_t fd, struct fs_handle_t* handle) {
	struct pcb_t* pcb = proc_data_get()->current_process;
	return array_list_set(pcb->fd_table, fd, handle);
}

void process_exit(uint64_t ec) {
	struct pcb_t* pcb = proc_data_get()->current_process;
	pcb->exit_code = ec;
	process_kill_current();
}

struct fs_handle_t* process_get_wd() {
	struct pcb_t* pcb = proc_data_get()->current_process;
	return pcb->wd;
}

void process_set_wd(struct fs_handle_t* handle) {
	struct pcb_t* pcb = proc_data_get()->current_process;
	pcb->wd = handle;
}

uint64_t process_get_mem_top(void) {
	struct pcb_t* pcb = proc_data_get()->current_process;
	return pcb->mem_top;
}

void process_set_mem_top(uint64_t top) {
	struct pcb_t* pcb = proc_data_get()->current_process;
	pcb->mem_top = top;
}

uint64_t process_get_cr3(void) {
	struct pcb_t* pcb = proc_data_get()->current_process;
	return pcb->cr3;
}

void process_set_cr3(uint64_t cr3) {
	struct pcb_t* pcb = proc_data_get()->current_process;
	pcb->cr3 = cr3;
}

uint64_t process_get_ppid(void) {
	struct pcb_t* pcb = proc_data_get()->current_process;
	lock_acquire(&lock_reap);
	uint64_t pid = pcb->parent->pid;
	lock_release(&lock_reap);
	return pid;
}

static uint8_t is_zombie(void* pcb) {
	struct pcb_t* child = pcb;

	return child->sched_cntr == SCHED_ZOMBIE;
}

uint64_t process_wait_pid(uint64_t pid, uint8_t no_hang, uint64_t* ec_ret) {
	struct pcb_t* pcb = proc_data_get()->current_process;

	uint64_t ec = 0;

	void* child;

	if (no_hang) {
		if (pid == -1uLL) {
			if (hash_table_find(pcb->child_table, &pid, &child, is_zombie)) {
				ec = process_reap_child(child);
			}
			else {
				ec = 0;
				pid = 0;
			}
		}
		else {
			if (!hash_table_get(pcb->child_table, pid, &child)) {
				return 0;
			}

			if (((struct pcb_t*)child)->sched_cntr == SCHED_ZOMBIE) {
				ec = process_reap_child(child);
			}
			else {
				ec = 0;
				pid = 0;
			}
		}
	}
	else {
		if (pid == -1uLL) {
			if (!hash_table_get_any(pcb->child_table, &pid, &child)) {
				return 0;
			}
		}
		else {
			if (!hash_table_get(pcb->child_table, pid, &child)) {
				return 0;
			}
		}

		ec = process_reap_child(child);
	}

	*ec_ret = ec;

	return 1;
}

void** process_get_meta(struct pcb_t* pcb, size_t num) {
	if (num > MAX_META) {
		return 0;
	}

	return pcb->meta;
}

void process_execve(struct pcb_t* actual, struct pcb_t* desired) {
	struct pcb_t temp;

	temp = *actual;
	*actual = *desired;

	scheduler_schedule(actual);

	temp.init_k_rsp_vaddr = 0;

	*desired = temp;
	process_discard(desired);
}

enum sched_cntr_t process_get_sched_cntr(struct pcb_t* pcb) {
	return pcb->sched_cntr;
}

void process_set_sched_cntr(struct pcb_t* pcb, enum sched_cntr_t cntr) {
	pcb->sched_cntr = cntr;
}

void process_set_next(struct pcb_t* pcb, struct pcb_t* next) {
	pcb->next = next;
}

struct pcb_t* process_get_next(struct pcb_t* pcb) {
	return pcb->next;
}

struct pcb_t** process_next_ref(struct pcb_t* pcb) {
	return &pcb->next;
}

uint64_t process_get_wake_time(struct pcb_t* pcb) {
	return pcb->sleep_state.wake_time;
}

void process_call_callback(struct pcb_t* pcb) {
	pcb->sleep_state.callback(pcb);
}

void process_resume_transfer(struct pcb_t* run) {
	struct proc_data_t* pd = proc_data_get();

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

struct pcb_t* process_create_userland_pcb(uint64_t rdi,
																					uint64_t rdx,
																				  uint64_t stack_vaddr,
																				  uint64_t stack_paddr,
																				  uint64_t cr3,
																				  struct pcb_t* source,
																				  uint64_t memtop,
																				  uint64_t pid) {
	struct fs_handle_t* wd;
	struct array_list_t* fd_table;
	struct hash_table_t* child_table;
	struct pcb_t* parent;
	struct signal_wait_t* monitor;

	if (source) {
		stack_vaddr = source->init_k_rsp_vaddr;
		stack_paddr = source->init_k_rsp_paddr;
		pid = source->pid;

		wd = source->wd;
		fd_table = source->fd_table;
		child_table = source->child_table;	
		parent = source->parent;
		monitor = source->monitor;
	}
	else {
		wd = fs_open("/", O_RDWR);
		fd_table = array_list_alloc(FD_INIT_SIZE, FD_GROWTH, 0);
		child_table = hash_table_alloc(CHILD_BUCKETS);
		parent = 0;
		monitor = signal_wait_alloc();
	}

	struct pcb_t* pcb = init_pcb((uint64_t)syscall_return,
															 wd,
															 fd_table,
															 child_table,
															 parent,
															 monitor,
															 cr3,
															 pid,
															 stack_vaddr,
															 stack_paddr,
															 memtop);

	pcb->rdi = rdi;
	pcb->rsi = pcb->rflags;
	pcb->rdx = rdx;

	if (pid == 1) {
		init_reaper_pcb = pcb;
	}

	return pcb;
}
