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

#include <lib/kmemcpy.h>

#include <drivers/acpi/tables.h>
#include <drivers/acpica_osl/init.h>
#include <drivers/pic_8259/pic.h>
#include <drivers/apic/apic_init.h>
#include <drivers/apic/ipi.h>
#include <drivers/ioapic/ioapic_init.h>

struct boot_context_t boot_context;

extern uint8_t ap_bootstrap_start;
extern uint8_t ap_bootstrap_end;

void kentry(void) {
	logging_log_debug("Kernel Entry");
	logging_log_debug("TSS and IDT init");
	tss_init();
	idt_init();
	logging_log_debug("TSS and IDT init done");

	logging_log_debug("ACPI init");
	acpi_copy_tables();
	logging_log_debug("ACPI init done");

	logging_log_debug("Initializing system clock");
	clock_src_init();
	time_init();
	logging_log_debug("System clock initialized.");

	logging_log_debug("APIC and IOAPIC init");
	pic_disab();
	apic_init();
	apic_timer_calib(apic_get_bsp_id());
	apic_nmi_enab();
	ioapic_init();
	logging_log_debug("APIC and IOAPIC init done");

	logging_log_debug("ACPICA init");
	acpica_init();
	logging_log_debug("ACPICA init done");

	logging_log_info("Boot Complete ModulOS");

	logging_log_info("Begining AP bootstrap sequence");
	kmemcpy(
			(void*)paging_ident(AP_ENTRY_PAGE * PAGE_SIZE_4K),
			&ap_bootstrap_start,
			(uint64_t)&ap_bootstrap_end - (uint64_t)&ap_bootstrap_start);

	apic_start_ap();

	logging_log_info("AP bootstrap sequence done");

	cpu_halt_loop();
}

void kapentry(uint64_t arb_id) {
	logging_log_debug("AP %lx bootstrap complete", arb_id);

	proc_data_set_id((uint8_t)arb_id);
	proc_data_get()->arb_id = (uint8_t)arb_id;

	alloc_init();

	logging_log_debug("AP TSS and IDT init");
	tss_init();
	idt_init_ap();
	logging_log_debug("AP TSS and IDT init done");

	logging_log_debug("AP APIC init");
	pic_disab();
	apic_init_ap();
	apic_timer_calib(apic_get_bsp_id());
	apic_nmi_enab();
	ioapic_init();
	logging_log_debug("AP APIC init done");

	logging_log_info("AP init complete");

	cpu_halt_loop();
}
