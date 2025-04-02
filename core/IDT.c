/* IDT.c - IDT structure */
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


#ifndef CORE_IDT_C
#define CORE_IDT_C

#include <stdint.h>

#include <core/memory.h>
#include <core/cpulowlevel.h>
#include <core/IDT.h>

#define KCODESEG		0x8
#define KDPL				0x0
#define UDPL				0x3

#define PRESENT			0x1
#define INVALID			0x0

#define TYPE_INT		0xE
#define TYPE_TRAP		0xF
#define TYPE_TSS		0x9

#define GRAN_BYTE		0x0

void idt_installisrs() {
	/* first create TSS so that ISTs work */
	struct TSS* volatile tss  = (struct TSS* volatile)kmalloc(kheap_shared, sizeof(struct TSS));
	struct TSSD* volatile tssd = (struct TSSD* volatile)&TSS_SEG;

	tssd->lim0 = (sizeof(struct TSS) - 1) & 0xFFFF;
	tssd->base0 = (uint64_t)tss & 0xFFFF;
	tssd->base1 = ((uint64_t)tss >> 16) & 0xFF;
	tssd->type = TYPE_TSS;
	tssd->dpl = UDPL;
	tssd->present = PRESENT;
	tssd->lim1 = ((sizeof(struct TSS) - 1) >> 16) & 0xF;
	tssd->gran = GRAN_BYTE;
	tssd->base2 = ((uint64_t)tss >> 24) & 0xFF;
	tssd->base3 = (uint32_t)((uint64_t)tss >> 32);

	tss->rsp00 = (uint32_t)((uint64_t)&rsp0) & 0xFFFFFFFF;
	tss->rsp01 = (uint32_t)((uint64_t)&rsp0 >> 32) & 0xFFFFFFFF;

	// TODO: add rings 1 and 2

	tss->ist10 = (uint32_t)((uint64_t)&rsp1) & 0xFFFFFFFF;
	tss->ist11 = (uint32_t)((uint64_t)&rsp1 >> 32) & 0xFFFFFFFF;

	tss->ist20 = (uint32_t)((uint64_t)&rsp2) & 0xFFFFFFFF;
	tss->ist21 = (uint32_t)((uint64_t)&rsp2 >> 32) & 0xFFFFFFFF;

	tss->ist30 = (uint32_t)((uint64_t)&rsp3) & 0xFFFFFFFF;
	tss->ist31 = (uint32_t)((uint64_t)&rsp3 >> 32) & 0xFFFFFFFF;

	tss->ist40 = (uint32_t)((uint64_t)&rsp4) & 0xFFFFFFFF;
	tss->ist41 = (uint32_t)((uint64_t)&rsp4 >> 32) & 0xFFFFFFFF;

	tss->ist50 = (uint32_t)((uint64_t)&rsp5) & 0xFFFFFFFF;
	tss->ist51 = (uint32_t)((uint64_t)&rsp5 >> 32) & 0xFFFFFFFF;

	tss->ist60 = (uint32_t)((uint64_t)&rsp6) & 0xFFFFFFFF;
	tss->ist61 = (uint32_t)((uint64_t)&rsp6 >> 32) & 0xFFFFFFFF;

	tss->ist70 = (uint32_t)((uint64_t)&rsp7) & 0xFFFFFFFF;
	tss->ist71 = (uint32_t)((uint64_t)&rsp7 >> 32) & 0xFFFFFFFF;

	// don't use iomap
	tss->iomap_addr = sizeof(struct TSS);

	ltr();

	struct IDTD* volatile idt = (struct IDTD* volatile)&IDT_BASE;

	uint64_t off = (uint64_t)&ISR_DE;
	//DE
	idt[0].off0 = off & 0xFFFF;
	idt[0].segsel = KCODESEG;
	idt[0].ist = 0;
	idt[0].type = TYPE_INT;
	idt[0].dpl = KDPL;
	idt[0].present = PRESENT;
	idt[0].off1 = (off >> 16) & 0xFFFF;
	idt[0].off2 = (uint32_t)(off >> 32);
}

#endif /* CORE_IDT_C */

