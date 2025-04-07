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
#include <core/IDT.h>
#include <core/mp.h>

#include <acpi/acpica.h>

#include <apic/lapic.h>
#include <apic/ioapic.h>

struct avail_memory_t kavail_memory;

void kentry(uint32_t mb2tag_ptr, uint32_t mb2magic) {
	if (mb2magic != MULTIBOOT2_MAGIC) {
		return; // Fatal, hang
	}

	/* get memmap */
	uint32_t total_size = ((struct mb2tag_fixed* volatile)(uint64_t)mb2tag_ptr)->total_size;
	uint32_t i;
	struct mb2tag_memmap* volatile memmap;
	uint32_t t0;

	/* parse tags */
	for (i = mb2tag_ptr + sizeof(struct mb2tag_fixed); i < mb2tag_ptr + total_size; i += ((struct mb2tag_start* volatile)(uint64_t)i)->size) {
		/* align to 8 byte */
		t0 = i % 8;
		i += t0 == 0 ? 0 : 8 - t0;

		/* check tag */
		switch (((struct mb2tag_start* volatile)(uint64_t)i)->type) {
			case MB2TAG_TYPE_END: //last tag
				i = mb2tag_ptr + total_size;
				break;

			case MB2TAG_TYPE_MEMMAP:
				memmap = (struct mb2tag_memmap* volatile)(uint64_t)i;
				kavail_memory.entries = memmap->entries;
				kavail_memory.length = (uint32_t)((memmap->size - sizeof(struct mb2tag_memmap) + sizeof(struct mb2tag_memmap_entry)) / memmap->entry_size);
				break;
			default:
				break; //ignore others
		}
	}

	/* set kernel instance to 0 */
	kPML4T = (PML4T_t volatile)&k0PML4T;

	/* init atomic */
	atomicinit();

	/* init serial */
	serialinit();

#ifdef DEBUG
	serialWriteStr(SERIAL1, "LOG: SERIAL 1\nSTATUS: Starting memory init...\n");
	serialWriteStr(SERIAL2, "LOG: SERIAL 2\nSTATUS: Starting memory init...\n");
#endif /* DEBUG */
	
	/* init memory manager */
	meminit();

#ifdef DEBUG
	serialWriteStr(SERIAL1, "STATUS: Memory init done\n");
	serialWriteStr(SERIAL2, "STATUS: Memory init done\n");
#endif /* DEBUG */

#ifdef DEBUG
	serialWriteStr(SERIAL1, "STATUS: Initializing interrupts...\n");
	serialWriteStr(SERIAL2, "STATUS: Initializing interrupts...\n");
#endif /* DEBUG */

	static uint64_t k0rsp[] = {(uint64_t)&rsp0, (uint64_t)&rsp1, (uint64_t)&rsp2, (uint64_t)&rsp3, (uint64_t)&rsp4, (uint64_t)&rsp5, (uint64_t)&rsp6, (uint64_t)&rsp7};
	idt_init();
	idt_installisrs((struct IDTD* volatile)&IDT_BASE, (uint64_t*)0x7000, &k0rsp[0]);
	loadidt();

#ifdef DEBUG
	serialWriteStr(SERIAL1, "STATUS: Interrupts done\n");
	serialWriteStr(SERIAL2, "STATUS: Interrupts done\n");
#endif /* DEBUG */

#ifdef DEBUG
	serialWriteStr(SERIAL1, "STATUS: Starting ACPICA subsystem init...\n");
	serialWriteStr(SERIAL2, "STATUS: Starting ACPICA subsystem init...\n");
#endif /* DEBUG */

	if(acpiinit() != 0) {
		panic(KPANIC_ACPI);
	}

#ifdef DEBUG
	serialWriteStr(SERIAL1, "STATUS: ACPICA subsystem init done\n");
	serialWriteStr(SERIAL2, "STATUS: ACPICA subsystem init done\n");
#endif /* DEBUG */

#ifdef DEBUG
	serialWriteStr(SERIAL1, "STATUS: Starting local APIC init...\n");
	serialWriteStr(SERIAL2, "STATUS: Starting local APIC init...\n");
#endif /* DEBUG */

	apic_initlocal();
	ksti();

#ifdef DEBUG
	serialWriteStr(SERIAL1, "STATUS: Local APIC init done\n");
	serialWriteStr(SERIAL2, "STATUS: Local APIC init done\n");
#endif /* DEBUG */

#ifdef DEBUG
	serialWriteStr(SERIAL1, "STATUS: Starting IO APIC init...\n");
	serialWriteStr(SERIAL2, "STATUS: Starting IO APIC init...\n");
#endif /* DEBUG */

	apic_initio();
	setInterrupts(0);
	ksti();


#ifdef DEBUG
	serialWriteStr(SERIAL1, "STATUS: IO APIC init done\n");
	serialWriteStr(SERIAL2, "STATUS: IO APIC init done\n");
#endif /* DEBUG */

#ifdef DEBUG
	serialWriteStr(SERIAL1, "STATUS: Starting MP init...\n");
	serialWriteStr(SERIAL2, "STATUS: Starting MP init...\n");
#endif /* DEBUG */

	mp_initall();

#ifdef DEBUG
	serialWriteStr(SERIAL1, "STATUS: All processors initialized\n");
	serialWriteStr(SERIAL2, "STATUS: All processors initialized\n");
#endif /* DEBUG */

#ifdef DEBUG
	serialWriteStr(SERIAL1, "STATUS: BSP init done\n");
	serialWriteStr(SERIAL2, "STATUS: BSP init done\n");
#endif /* DEBUG */

	panic(KPANIC_UNK);
}

void kapentry() {
#ifdef DEBUG
	serialWriteStr(SERIAL1, "STATUS: Starting AP core init...\n");
	serialWriteStr(SERIAL2, "STATUS: Starting AP core init...\n");
#endif /* DEBUG */

#ifdef DEBUG
	serialWriteStr(SERIAL1, "STATUS: Initializing interrupts...\n");
	serialWriteStr(SERIAL2, "STATUS: Initializing interrupts...\n");
#endif /* DEBUG */
	uint64_t rsp[8] = {
		(uint64_t)kmalloc(0x4000),
		(uint64_t)kmalloc(0x1000),
		(uint64_t)kmalloc(0x1000),
		(uint64_t)kmalloc(0x1000),
		(uint64_t)kmalloc(0x80),
		(uint64_t)kmalloc(0x80),
		(uint64_t)kmalloc(0x80),
		(uint64_t)kmalloc(0x80)};
	idt_installisrs((struct IDTD* volatile)&IDT_BASE, (uint64_t*)mp_gdtptr.off, &rsp[0]);
	loadidt();

#ifdef DEBUG
	serialWriteStr(SERIAL1, "STATUS: Interrupts done\n");
	serialWriteStr(SERIAL2, "STATUS: Interrupts done\n");
#endif /* DEBUG */

#ifdef DEBUG
	serialWriteStr(SERIAL1, "STATUS: Starting local APIC init...\n");
	serialWriteStr(SERIAL2, "STATUS: Starting local APIC init...\n");
#endif /* DEBUG */

	apic_initlocalap((uint64_t*)&IDT_BASE);
	ksti();

#ifdef DEBUG
	serialWriteStr(SERIAL1, "STATUS: Local APIC init done\n");
	serialWriteStr(SERIAL2, "STATUS: Local APIC init done\n");
#endif /* DEBUG */

	/* let bsp know that ap is done initializing and ready to receive tasks */
	mp_loading = MP_LOADING_IDLE;

#ifdef DEBUG
	serialWriteStr(SERIAL1, "STATUS: AP init done\n");
	serialWriteStr(SERIAL2, "STATUS: AP init done\n");
#endif /* DEBUG */

	panic(KPANIC_UNK);
}

#endif /* CORE_KENTRY_C */
