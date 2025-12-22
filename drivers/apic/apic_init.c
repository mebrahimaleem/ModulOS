/* apic_init.h - Advanced programmable interrupt controller initialization */
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

#include <stdint.h>


#include <apic/apic_init.h>

#include <kernel/core/logging.h>
#include <kernel/core/msr.h>
#include <kernel/core/alloc.h>
#include <kernel/core/paging.h>
#include <kernel/core/mm.h>
#include <kernel/core/ports.h>

#define APIC_BASE_MASK	0xFFFFFFFFFF000
#define APIC_AE					0x800

#define OFF_APIC_ID		0x0020
#define OFF_APIC_EFR	0x0400
#define OFF_APIC_SPUR	0x00F0

#define OFF_APIC_TMR_LVT	0x320
#define OFF_APIC_THR_LVT	0x330
#define OFF_APIC_PRF_LVT	0x340
#define OFF_APIC_LI0_LVT	0x350
#define OFF_APIC_LI1_LVT	0x360
#define OFF_APIC_ERR_LVT	0x370

#define OFF_APIC_EI_BASE	0x500
#define APIC_LVT_SIZE			0x10

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

// pic master spurious (irq 7, int 0x27) works for apic spurious as well
#define PIC_SPURIOUS_VEC	0x27
#define APIC_ASE					0x100
#define NMI_VECTOR				0x02

#define CMOS_RAM_INDEX	0x70
#define CMOS_RAM_DATA		0x71
#define NMI_DISAB_MASK	0x80

struct apic_t {
	uint64_t v_base;
};

static struct apic_t* apics __attribute__((unused));

struct lvt_register_t {
	uint8_t v;
	uint8_t flg;
	uint8_t msk;
	uint8_t resv;
} __attribute__((packed));

void apic_init(void) {
	struct apic_t* bs_apic = kmalloc(sizeof(struct apic_t));
	bs_apic->v_base = mm_alloc_dv(MM_ORDER_4K);
	paging_map(bs_apic->v_base, msr_read(MSR_APIC_BASE) & APIC_BASE_MASK,
			PAGE_PRESENT | PAGE_RW | PAT_MMIO_4K, PAGE_4K);

	const uint32_t ver = *(volatile uint32_t*)(bs_apic->v_base + OFF_APIC_ID);
	logging_log_info("Initializing Local APIC 0x%X64\r\nMasking all APIC interrupts",
			(uint64_t)(ver >> APIC_ID_SHFT));

	// mask all interrupts for now
	
	// check eas, if set then mask ext lvt based on xlc
	if ((ver & APIC_VER_EAS) == APIC_VER_EAS) {
		logging_log_debug("Detected EAS bit");
		for (uint8_t xlc = 
				(*(volatile uint32_t*)(bs_apic->v_base + OFF_APIC_EFR) >> APIC_XLC_SHFT) & APIC_XLC_MASK;
				xlc > 0; xlc--) {
			*(volatile struct lvt_register_t*)(bs_apic->v_base + OFF_APIC_EI_BASE + APIC_LVT_SIZE * xlc) = 
			(struct lvt_register_t){
				.msk = APIC_LVT_MASK,
				.resv = 0
			};
		}
	}

	for (uint64_t addr = bs_apic->v_base + OFF_APIC_TMR_LVT;
			addr <= bs_apic->v_base + OFF_APIC_ERR_LVT; addr += APIC_LVT_SIZE) {
		*(volatile struct lvt_register_t*)(addr) = (struct lvt_register_t) {
			.msk = APIC_XLC_MASK,
			.resv = 0
		};
	}

	// set lints
	*(volatile struct lvt_register_t*)(bs_apic->v_base + OFF_APIC_LI0_LVT) = (struct lvt_register_t) {
		.flg = APIC_LVT_MT_EXT,
		.msk = 0,
		.resv = 0
	};

	*(volatile struct lvt_register_t*)(bs_apic->v_base + OFF_APIC_LI1_LVT) = (struct lvt_register_t) {
		.v = NMI_VECTOR,
		.flg = APIC_LVT_MT_NMI | APIC_LVT_TRG_EDGE,
		.msk = 0,
		.resv = 0
	};

	// enable apic
	*(volatile uint32_t*)(bs_apic->v_base + OFF_APIC_SPUR) = PIC_SPURIOUS_VEC | APIC_ASE;
	msr_write(MSR_APIC_BASE, msr_read(MSR_APIC_BASE) | (uint64_t)APIC_AE);

	logging_log_info("APIC LVTs masked. NMIs routed to 0x%X64", (uint64_t)NMI_VECTOR);
}

void apic_nmi_enab(void) {
	outb(CMOS_RAM_INDEX, inb(CMOS_RAM_INDEX) & NMI_DISAB_MASK);
	io_wait();
	inb(CMOS_RAM_DATA);
}


void apic_disab(void) {
	msr_write(MSR_APIC_BASE, msr_read(MSR_APIC_BASE) & ~(uint64_t)APIC_AE);
}
