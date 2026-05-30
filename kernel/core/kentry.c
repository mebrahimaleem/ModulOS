/* kentry.c - kernel entry point */
/* Copyright (C) 2025-2026  Ebrahim Aleem
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

#include <core/kentry.h>
#include <core/paging.h>
#include <core/alloc.h>
#include <core/tss.h>
#include <core/idt.h>
#include <core/cpu_instr.h>
#include <core/logging.h>
#include <core/clock_src.h>
#include <core/time.h>
#include <core/proc_data.h>
#include <core/scheduler.h>
#include <core/process.h>
#include <core/lock.h>
#include <core/fs.h>
#include <core/msr.h>
#include <core/gdt.h>
#include <core/syscall.h>
#include <core/mm.h>
#include <core/elf.h>
#include <core/lock.h>
#include <core/panic.h>

#include <lib/kmemcpy.h>

#include <devfs/tty.h>

#ifdef MEM_TEST
#include <mem_test/alloc_test.h>
#endif /* MEM_TEST */

#include <acpi/tables.h>
#include <apic/apic_init.h>
#include <apic/ipi.h>

#include <pic_8259/pic.h>

#include <ioapic/ioapic_init.h>

#include <drivers/pcie/pcie_init.h>
#include <drivers/disk/disk.h>

#define RFL_MASK	0xD5

#define CR4_FSGSBASE		(1 << 16)

struct boot_context_t boot_context;

extern uint8_t ap_bootstrap_start;
extern uint64_t* init_stacks;
extern uint8_t ap_bootstrap_end;
extern struct gdt_t(** ap_gdts)[GDT_NUM_ENTRIES];
extern uint8_t* ap_init_locks;

extern uint64_t init_stack_vaddr;
extern uint64_t init_stack_paddr;

extern uint64_t* init_stacks_paddr;
extern uint64_t* init_stacks_vaddr;

static uint8_t prepare_userland_lock;
static uint8_t init_done;

static inline void write_syscall_msr(void) {
	msr_write(MSR_STAR, ((GDT_USER_CS - 0x10) << 48) | (GDT_KERNEL_CS << 32));
	msr_write(MSR_LSTAR, (uint64_t)syscall_entry);
	msr_write(MSR_FMASK, RFL_MASK);
}

#ifdef DEBUG_LOGGING_MEM
#define LOG_DELAY_MS	1000

__attribute__((noreturn)) static void periodic_logging(void* _ign) {
	(void)_ign;

	while (1) {
		mm_log_usage();
		alloc_log_usage();

		time_sleep(LOG_DELAY_MS);
	}
}
#endif /* DEBUG_LOGGING_MEM */

void kentry(void) {
	logging_log_debug("Kernel Entry");

#ifdef MEM_TEST
	logging_log_debug("Allocator test");
	alloc_test_assert();
	logging_log_debug("Allocator test pass");
#endif /* MEM_TEST */

	logging_log_debug("TSS and IDT init");
	tss_init((void*)paging_ident((uint64_t)boot_context.gdt));
	process_init(init_stack_vaddr, init_stack_paddr);
	idt_init();
	paging_ensure_mapped();
	scheduler_init();

	init_done = 0;
	lock_init(&prepare_userland_lock);

	write_syscall_msr();
	cpu_set_cr4(CR4_FSGSBASE);
	cpu_init_fx();

	logging_log_debug("TSS and IDT init done");

	logging_log_debug("ACPI init");
	acpi_copy_tables();
	logging_log_debug("ACPI init done");

	logging_log_debug("Initializing system clock");
	clock_src_init();
	time_init();
	logging_log_debug("System clock initialized.");

	logging_log_debug("APIC and IOAPIC init");
	apic_ipi_init();
	pic_disab();
	apic_init();
	apic_timer_calib(apic_get_bsp_id());
	apic_nmi_enab();
	ioapic_init();
	apic_init_shootdowns();
	cpu_sti();
	logging_log_debug("APIC and IOAPIC init done");

	proc_data_get()->sts |= PROC_STS_INT_READY;

	logging_log_debug("Early PCIE init");
	disk_init();
	fs_init();
	tty_init();
	pcie_init();
	pcie_enumerate();
	logging_log_debug("Early PCIE init done");

	logging_log_info("Boot Complete ModulOS");

	logging_log_info("Begining AP bootstrap sequence");
	kmemcpy(
			(void*)paging_ident(AP_ENTRY_PAGE * PAGE_SIZE_4K),
			&ap_bootstrap_start,
			(uint64_t)&ap_bootstrap_end - (uint64_t)&ap_bootstrap_start);

#ifdef SMP_ENABLE
	apic_start_ap();
#endif /* SMP_ENABLE */

	logging_log_info("AP bootstrap sequence done");

	mm_transaction_init();
	process_init_reaper();

#ifdef DEBUG_LOGGING_MEM
	scheduler_schedule(process_from_func(periodic_logging, 0));
#endif /* DEBUG_LOGGING_MEM */

	process_kill_current();
}

void kapentry(uint64_t arb_id) {
	lock_release(&ap_init_locks[arb_id]);
	logging_log_debug("AP %lx bootstrap complete", arb_id);

	proc_data_set_id((uint8_t)arb_id);
	proc_data_get()->arb_id = (uint8_t)arb_id;

	write_syscall_msr();
	cpu_set_cr4(CR4_FSGSBASE);
	cpu_init_fx();

	alloc_init();

	logging_log_debug("AP TSS and IDT init");
	tss_init(ap_gdts[proc_data_get()->arb_id]);
	process_init_ap(init_stacks_vaddr[arb_id], init_stacks_paddr[arb_id]);
	idt_init_ap();
	logging_log_debug("AP TSS and IDT init done");

	logging_log_debug("AP APIC init");
	apic_init_ap();
	apic_timer_calib(apic_get_bsp_id());
	apic_nmi_enab();
	cpu_sti();
	logging_log_debug("AP APIC init done");

	proc_data_get()->sts |= PROC_STS_INT_READY;

	logging_log_info("AP init complete");

	process_kill_current();
}

void prepare_userland(void* cntx) {
	(void)cntx;

	struct fs_handle_t* f = fs_open("/test.txt", FILE_FLAGS_READ | FILE_FLAGS_WRITE | FILE_FLAGS_CREATE);
	if (f == 0) {
		logging_log_error("Failed to create /test.txt");
	}
	fs_write(f, "Hi\n", 3);
	fs_close(f);

	lock_acquire(&prepare_userland_lock);
	if (init_done) {
		logging_log_error("Multiple calls to prepare userland");
		panic(PANIC_STATE);
	}

	init_done = 1;
	lock_release(&prepare_userland_lock);

	struct fs_handle_t* shell = fs_open("/bin/shell", FILE_FLAGS_READ);
	if (!shell) {
		logging_log_error("Failed to open shell file");
	}

	else {
		struct pcb_t* shell_pcb = elf_load(shell, process_assign_pid(), "/bin/shell ModulOS", "USER=root PWD=/");
		if (!shell_pcb) {
			logging_log_error("Failed to load shell file");
		}
		fs_close(shell);

		scheduler_schedule(shell_pcb);
	}
}
