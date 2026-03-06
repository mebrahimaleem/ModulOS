/* apic_init.c - Advanced programmable interrupt controller initialization interface */
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

#include <apic/apic_init.h>
#include <apic/apic_regs.h>
#include <apic/isr.h>
#include <acpi/tables.h>
#include <apic/ipi.h>

#include <kernel/core/logging.h>
#include <kernel/core/msr.h>
#include <kernel/core/paging.h>
#include <kernel/core/ports.h>
#include <kernel/core/idt.h>
#include <kernel/core/cpu_instr.h>
#include <kernel/core/panic.h>
#include <kernel/core/time.h>
#include <kernel/core/lock.h>
#include <kernel/core/clock_src.h>
#include <kernel/core/kentry.h>
#include <kernel/core/proc_data.h>
#include <kernel/core/mm.h>
#include <kernel/core/gdt.h>
#include <kernel/core/alloc.h>

#include <kernel/lib/kmemcpy.h>

#define APIC_BASE_MASK	0xFFFFFFFFFF000

#define APIC_ID_SHFT	24
#define APIC_VER_EAS	0x80000000
#define APIC_XLC_SHFT	16
#define APIC_XLC_MASK	8

#define APIC_LVT_MT_FIXED	0x0
#define APIC_LVT_MT_SMI		0x2
#define APIC_LVT_MT_NMI		0x4
#define APIC_LVT_MT_EXT		0x7

#define APIC_LVT_TRG_EDGE	0x00
#define APIC_LVT_TRG_LEVL	0x80

#define APIC_LVT_MASK			0x01
#define APIC_LVT_TMR_PER	0x02
#define APIC_LVT_TMR_ONE	0x00

#define APIC_DIV_CFG_OFF	0x3E0
#define APIC_INI_CNT_OFF	0x380
#define APIC_CUR_CNT_OFF	0x390

#define APIC_CAL_ITR_MS		50
#define APIC_CAL_BATCH		20
#define APIC_CAL_TOL			8000

#define APIC_CLOCK_MS			50

// pic master spurious (irq 7, int 0x27) works for apic spurious as well
#define PIC_SPURIOUS_VEC	0x27
#define APIC_ASE					0x100

#define CMOS_RAM_INDEX	0x70
#define CMOS_RAM_DATA		0x71
#define NMI_DISAB_MASK	0x7F

#define APIC_TMR_DIV_1	0xB

#define LINT0_SET	0x1
#define LINT1_SET	0x2

#define AP_ARB_BASE	0x9000

uint64_t apic_base;
uint8_t bsp_apic_id;
static uint8_t num_apic;

static uint8_t timer_vector;
static uint8_t error_vector;

extern uint8_t gdt;
extern uint8_t gdt_end;
extern uint8_t kernel_pml4;

uint64_t* init_stacks;
volatile struct gdt_t(** ap_gdts)[GDT_NUM_ENTRIES];
volatile struct gdt_ptr_64_t** ap_gdt_ptr_64;
uint8_t* ap_init_locks;

void apic_init(void) {
	apic_base = msr_read(MSR_APIC_BASE) & APIC_BASE_MASK;
	if (!paging_map(apic_base, apic_base,
			PAGE_PRESENT | PAGE_RW | PAT_MMIO_4K, PAGE_4K)) {
		logging_log_error("Failed to map memory for LAPIC");
		panic(PANIC_PAGING);
	}

	timer_vector = idt_get_vector();
	error_vector = idt_get_vector();

	// install idts
	idt_install(timer_vector, (uint64_t)apic_isr_timer, GDT_CODE_SEL, IST_SCHED, IDT_GATE_INT, 0);
	idt_install(error_vector, (uint64_t)apic_isr_error, GDT_CODE_SEL, 0, IDT_GATE_INT, 0);

	apic_init_ap();

	// init stacks
	init_stacks = kmalloc(sizeof(uint64_t*) * num_apic);
	proc_data_ptr = kmalloc(sizeof(struct proc_data_t*) * num_apic);
	proc_data_ptr[0] = &bsp_proc_data;

	// init gdts
	ap_gdts = kmalloc(sizeof(struct gdt_t(*)[GDT_NUM_ENTRIES]) * num_apic);
	ap_gdt_ptr_64 = kmalloc(sizeof(struct gdt_ptr_64_t*) * num_apic);
	ap_init_locks = kmalloc(sizeof(uint8_t) * num_apic);

	for (--num_apic; num_apic; num_apic--) {
		proc_data_ptr[num_apic] = kmalloc(sizeof(struct proc_data_t));
		init_stacks[num_apic] = (uint64_t)kmalloc(INIT_STACK_SIZE) + INIT_STACK_SIZE;

		ap_gdts[num_apic] = kmalloc(sizeof(struct gdt_t[GDT_NUM_ENTRIES]));
		ap_gdt_ptr_64[num_apic] = kmalloc(sizeof(struct gdt_ptr_64_t));

		kmemcpy((void*)ap_gdts[num_apic], &gdt, (uint64_t)&gdt_end - (uint64_t)&gdt);
		ap_gdt_ptr_64[num_apic]->addr = (uint64_t)ap_gdts[num_apic];
		ap_gdt_ptr_64[num_apic]->limit = (uint16_t)((uint64_t)&gdt_end - (uint64_t)&gdt - 1);

		lock_init(&ap_init_locks[num_apic]);
		lock_acquire(&ap_init_locks[num_apic]);
	}
}

void apic_init_ap(void) {
	const uint8_t apic_id = (uint8_t)(apic_read_reg(APIC_REG_IDR) >> APIC_ID_SHFT);
	bsp_apic_id = apic_id;
	logging_log_info("Initializing Local APIC 0x%lX", (uint64_t)apic_id);

	// get ACPI uid
	num_apic = 0;
	uint64_t handle;
	uint8_t acpi_uid = ACPI_UID_ALL_PROC;
	struct acpi_madt_ics_local_apic_t* local_apic;
	acpi_parse_madt_ics_start(&handle);
	for (acpi_parse_madt_ics((void*)&local_apic, &handle, MADT_ICS_PROCESSOR_LOCAL_APIC);
			handle != 0;
			acpi_parse_madt_ics((void*)&local_apic, &handle, MADT_ICS_PROCESSOR_LOCAL_APIC)) {
		if (local_apic->APICID == apic_id) {
			acpi_uid = local_apic->ACPIProcessorUID;
			logging_log_debug("ACPI UID 0x%lX -> APIC ID 0x%lX", (uint64_t)acpi_uid, (uint64_t)apic_id);
		}
		num_apic++;
	}

	if (acpi_uid == ACPI_UID_ALL_PROC) {
		logging_log_error("No ACPI ID for APIC 0x%lX", (uint64_t)apic_id);
		panic(PANIC_APIC);
	}

	// mask all interrupts for now
	
	// check eas, if set then mask ext lvt based on xlc
	if ((apic_read_reg(APIC_REG_VER) & APIC_VER_EAS) == APIC_VER_EAS) {
		const uint8_t xlc = (apic_read_reg(APIC_REG_EFR) >> APIC_XLC_SHFT) & APIC_XLC_MASK;
		logging_log_debug("Found %lu extended lvt registers", (uint64_t)xlc);
		if (xlc >= 1) {
			apic_write_lve(APIC_REG_EE0, 0, 0, APIC_LVT_MASK);
		}
		if (xlc >= 2) {
			apic_write_lve(APIC_REG_EE1, 0, 0, APIC_LVT_MASK);
		}
		if (xlc >= 3) {
			apic_write_lve(APIC_REG_EE2, 0, 0, APIC_LVT_MASK);
		}
		if (xlc >= 4) {
			apic_write_lve(APIC_REG_EE3, 0, 0, APIC_LVT_MASK);
		}
	}

	// set NMI
	uint8_t lint_state = 0;
	struct acpi_madt_ics_local_apic_nmi_t* local_nmi;
	acpi_parse_madt_ics_start(&handle);
	for (acpi_parse_madt_ics((void*)&local_nmi, &handle, MADT_ICS_LOCAL_APIC_NMI);
			handle != 0;
			acpi_parse_madt_ics((void*)&local_nmi, &handle, MADT_ICS_LOCAL_APIC_NMI)) {
		if (local_nmi->ACPIProcessorUID == acpi_uid || local_nmi->ACPIProcessorUID == ACPI_UID_ALL_PROC) {
			logging_log_debug("Found local NMI on LINT%lu", (uint64_t)local_nmi->LocalAPICLINTNum);
			const uint8_t tgm = 
				((local_nmi->Flags.PolarityAndTriggerMode & MADT_ICS_MPS_TRIGGER_MASK) == MADT_ICS_MPS_TRIGGER_LEVL) ?
				APIC_LVT_TRG_LEVL : APIC_LVT_TRG_EDGE; // default to edge since its more probable
			switch (local_nmi->LocalAPICLINTNum) {
				case 0:
					lint_state |= LINT0_SET;
					apic_write_lve(APIC_REG_L0E, 0,
							APIC_LVT_MT_NMI | tgm, 0); // nmi, v is ignored
					break;
				case 1:
					lint_state |= LINT1_SET;
					apic_write_lve(APIC_REG_L1E, 0,
							APIC_LVT_MT_NMI | tgm, 0); // nmi, v is ignored
					break;
				default:
					logging_log_error("NMI on non-existent LINT%lu", (uint64_t)local_nmi->LocalAPICLINTNum);
					panic(PANIC_APIC);
			}
		}
	}

	// set LVT entries
	
	if ((lint_state & LINT0_SET) != LINT0_SET) {
		apic_write_lve(APIC_REG_L0E, 0, APIC_LVT_MT_FIXED | APIC_LVT_TRG_EDGE, APIC_LVT_MASK);
	}

	if ((lint_state & LINT1_SET) != LINT1_SET) {
		apic_write_lve(APIC_REG_L1E, 0, APIC_LVT_MT_FIXED | APIC_LVT_TRG_EDGE, APIC_LVT_MASK);
	}

	apic_write_lve(APIC_REG_TME, timer_vector,
			APIC_LVT_MT_FIXED | APIC_LVT_TRG_EDGE, APIC_LVT_TMR_ONE | APIC_LVT_MASK);

	apic_write_lve(APIC_REG_THE, 0,
			0, APIC_LVT_MASK);

	apic_write_lve(APIC_REG_PRE, 0,
			0, APIC_LVT_MASK);

	apic_write_lve(APIC_REG_ERE, error_vector,
			APIC_LVT_MT_FIXED | APIC_LVT_TRG_EDGE, 0);

	// enable apic
	apic_write_reg(APIC_REG_ESR, 0);
	apic_write_reg(APIC_REG_SPR, PIC_SPURIOUS_VEC | APIC_ASE); // pic and apic spurious both only iret, so reuse
}

void apic_nmi_enab(void) {
	outb(CMOS_RAM_INDEX, inb(CMOS_RAM_INDEX) & NMI_DISAB_MASK);
	io_wait();
	inb(CMOS_RAM_DATA);
}

void apic_timer_calib(uint8_t id) {
	uint8_t i;
	uint64_t min, max, sum, ticks, rate, delta;

	*(volatile uint32_t*)(apic_base + APIC_DIV_CFG_OFF) = APIC_TMR_DIV_1;

	do {
		min = 0xFFFFFFFFFFFFFFFFu;
		max = 0;
		sum = 0;
		for (i = 0; i < APIC_CAL_BATCH; i++) {
			*(volatile uint32_t*)(apic_base + APIC_INI_CNT_OFF) = 0xFFFFFFFF;

			(void)*(volatile uint32_t*)(apic_base + id);

			delta = time_busy_wait(APIC_CAL_ITR_MS * TIME_CONV_MS_TO_NS);

			ticks = 0xFFFFFFFF - *(volatile uint32_t*)(apic_base + APIC_CUR_CNT_OFF);
			rate = delta / ticks;

			if (rate < min) {
				min = rate;
			}

			if (rate > max) {
				max = rate;
			}

			sum += rate;
		}
	} while (max - min > APIC_CAL_TOL);

	rate = sum / APIC_CAL_BATCH;
	logging_log_debug("Apic timer rate: %lu fs/tick (%lu/%lu)", rate, min, max);

	apic_write_lve(APIC_REG_TME, timer_vector,
			APIC_LVT_MT_FIXED | APIC_LVT_TRG_EDGE, APIC_LVT_TMR_PER);

	*(volatile uint32_t*)(apic_base + APIC_INI_CNT_OFF) =
		(uint32_t)(APIC_CLOCK_MS * TIME_CONV_MS_TO_FS / rate);

	(void)*(volatile uint32_t*)(apic_base + id);
}

uint8_t apic_get_bsp_id(void) {
	return bsp_apic_id;
}

void apic_start_ap(void) {
	*(volatile uint8_t*)paging_ident(AP_ARB_BASE + 0x0) = 0; // arb lock
	*(volatile uint8_t*)paging_ident(AP_ARB_BASE + 0x1) = 0; // arb id
	*(volatile uint16_t*)paging_ident(AP_ARB_BASE + 0x2) = 
		(uint16_t)((uint64_t)&gdt_end - (uint64_t)&gdt - 1); // gdt32 ptr
	*(volatile uint32_t*)paging_ident(AP_ARB_BASE + 0x4) = (uint32_t)(uint64_t)&gdt;
	*(volatile uint16_t*)paging_ident(AP_ARB_BASE + 0x8) = 
		(uint16_t)((uint64_t)&gdt_end - (uint64_t)&gdt - 1); // gdt64 ptr
	*(volatile uint64_t*)paging_ident(AP_ARB_BASE + 0xa) = (uint64_t)&gdt;
	*(volatile uint32_t*)paging_ident(AP_ARB_BASE + 0x12) = (uint32_t)(uint64_t)&kernel_pml4;
	*(volatile uint64_t*)paging_ident(AP_ARB_BASE + 0x16) = (uint64_t)init_stacks;

	uint64_t handle;
	struct acpi_madt_ics_local_apic_t* local_apic;
	acpi_parse_madt_ics_start(&handle);
	for (acpi_parse_madt_ics((void*)&local_apic, &handle, MADT_ICS_PROCESSOR_LOCAL_APIC);
			handle != 0;
			acpi_parse_madt_ics((void*)&local_apic, &handle, MADT_ICS_PROCESSOR_LOCAL_APIC)) {
		if (local_apic->APICID == apic_get_bsp_id()) {
			continue;
		}
		apic_send_ipi_init_set(local_apic->APICID);
		apic_wait_for_ipi();
		apic_send_ipi_init_clear(local_apic->APICID);

		time_busy_wait(10 * TIME_CONV_MS_TO_NS);
		apic_send_ipi_sipi(local_apic->APICID);

		time_busy_wait(2000);
		apic_send_ipi_sipi(local_apic->APICID);
	}
}
