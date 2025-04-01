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

#include <acpi/acpica.h>

#include <apic/lapic.h>

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

#ifdef DEBUG
	serialWriteStr(SERIAL1, "STATUS: Local APIC init done\n");
	serialWriteStr(SERIAL2, "STATUS: Local APIC init done\n");
#endif /* DEBUG */

#ifdef DEBUG
	serialWriteStr(SERIAL1, "STATUS: Core init done\n");
	serialWriteStr(SERIAL2, "STATUS: Core init done\n");
#endif /* DEBUG */

	panic(KPANIC_UNK);
	return;
}

#endif /* CORE_KENTRY_C */
