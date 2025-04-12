/* kentry.c - kernel entry point */
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

#ifndef CORE_KENTRY_C
#define CORE_KENTRY_C

#include <multiboot2/multiboot2.h>
#include <multiboot2/tags.h>

#include <core/kentry.h>
#include <core/atomic.h>
#include <core/serial.h>
#include <core/panic.h>
#include <core/memory.h>
#include <core/idt.h>
#include <core/mp.h>
#include <core/scheduler.h>
#include <core/threads.h>

#include <acpi/acpica.h>

#include <apic/lapic.h>
#include <apic/ioapic.h>

struct avail_memory_t kavail_memory;

void kentry(uint32_t mb2tag_ptr, uint32_t mb2magic) {
	if (mb2magic != MULTIBOOT2_MAGIC) {
		return; // Fatal, hang
	}

	/* get memmap */
	uint32_t total_size = ((volatile struct mb2tag_fixed* )(uint64_t)mb2tag_ptr)->total_size;
	uint32_t i;
	volatile struct mb2tag_memmap* memmap;
	uint32_t t0;

	/* parse tags */
	for (i = mb2tag_ptr + sizeof(struct mb2tag_fixed); i < mb2tag_ptr + total_size; i += ((volatile struct mb2tag_start* )(uint64_t)i)->size) {
		/* align to 8 byte */
		t0 = i % 8;
		i += t0 == 0 ? 0 : 8 - t0;

		/* check tag */
		switch (((volatile struct mb2tag_start* )(uint64_t)i)->type) {
			case MB2TAG_TYPE_END: //last tag
				i = mb2tag_ptr + total_size;
				break;

			case MB2TAG_TYPE_MEMMAP:
				memmap = (volatile struct mb2tag_memmap* )(uint64_t)i;
				kavail_memory.entries =(struct mb2tag_memmap_entry*)memmap->entries;
				kavail_memory.length = (uint32_t)((memmap->size - sizeof(struct mb2tag_memmap) + sizeof(struct mb2tag_memmap_entry)) / memmap->entry_size);
				break;
			default:
				break; //ignore others
		}
	}

	/* set kernel instance to 0 */
	kPML4T = (volatile PML4T_t )&k0PML4T;

	/* init atomic */
	atomicinit();

	/* init serial */
	serialinit();

	/* cant use fancy logging until memory is initialized */
#ifdef DEBUG
	serialWriteStr(SERIAL1, "\n\n[INFO]\tStarting BSP init...\n[INFO]\tStarting memory init...\n");
	serialWriteStr(SERIAL2, "\n\n[INFO]\tStarting BSP init...\n[INFO]\tStarting memory init...\n");
#endif /* DEBUG */
	
	/* init memory manager */
	initpaging();
	meminit();

	INFO_LOG("Memory init done");

	INFO_LOG("Starting interrupts init...");

	static uint64_t k0rsp[] = {(uint64_t)&rsp0, (uint64_t)&rsp1, (uint64_t)&rsp2, (uint64_t)&rsp3, (uint64_t)&rsp4, (uint64_t)&rsp5, (uint64_t)&rsp6, (uint64_t)&rsp7};
	idt_init();
	idt_installisrs((volatile struct IDTD* )&IDT_BASE, (uint64_t*)0x7000, &k0rsp[0]);
	loadidt();

	INFO_LOG("Interrupts init done");

	INFO_LOG("Starting ACPICA first stage init...");

	if(acpiinit() != 0) {
		panic(KPANIC_ACPI);
	}

	INFO_LOG("ACPICA first stage init done");

	INFO_LOG("Starting local APIC init...");
	
	apic_initlocal();
	ksti();

	INFO_LOG("Local APIC init done");

	INFO_LOG("Starting IO/APIC init...");

	apic_initio();
	setInterrupts(0);
	ksti();
	
	INFO_LOG("IO/APIC init done");

	INFO_LOG("Starting MP init...");

	scheduler_init();
	thread_init();
	mp_initall();
	
	/* calibrate local apic timer */
	apic_lapic_calibrateTimer();

	INFO_LOG("MP init done");

	INFO_LOG("BSP init done");

	INFO_LOG("Calling scheduler");
	scheduler_nextTask();
}

void kapentry() {
	INFO_LOG("Starting AP init...");

	INFO_LOG("Starting interrupts init...");

	uint64_t rsp[8] = {
		mp_rsp,
		(uint64_t)kmalloc(0x1000) + 0x1000,
		(uint64_t)kmalloc(0x1000) + 0x1000,
		(uint64_t)kmalloc(0x1000) + 0x1000,
		(uint64_t)kmalloc(0x80) + 0x80,
		(uint64_t)kmalloc(0x80) + 0x80,
		(uint64_t)kmalloc(0x80) + 0x80,
		(uint64_t)kmalloc(0x1000) + 0x1000};
	idt_installisrs((volatile struct IDTD* )&IDT_BASE, (uint64_t*)mp_gdtptr.off, &rsp[0]);
	loadidt();

	INFO_LOG("Interrupts init done");

	INFO_LOG("Starting local APIC init...");

	apic_initlocalap();
	ksti();

	/* calibrate local apic timer */
	apic_lapic_calibrateTimer();

	/* let bsp know that ap is done initializing and bsp can continue */
	mp_loading = MP_LOADING_IDLE;

	INFO_LOG("Local APIC init done");

	INFO_LOG("AP init done");

	INFO_LOG("Calling scheduler");
	scheduler_nextTask();
}

#endif /* CORE_KENTRY_C */
