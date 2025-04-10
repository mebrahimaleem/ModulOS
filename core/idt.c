/* idt.c - IDT structure */
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
#include <core/idt.h>

#define TYPE_TSS		0x9

#define GRAN_BYTE		0x0

#define IDTDE(base, i) \
	idt[i].off0 = ((uint64_t)&base) & 0xFFFF; \
	idt[i].segsel = IDT_KCODESEG; \
	idt[i].ist = 0; \
	idt[i].type = IDT_TYPE_INT; \
	idt[i].dpl = IDT_KDPL; \
	idt[i].present = IDT_PRESENT; \
	idt[i].off1 = (((uint64_t)&base) >> 16) & 0xFFFF; \
	idt[i].off2 = (uint32_t)(((uint64_t)&base) >> 32);

#define IDTDE2(base, i, is) \
	idt[i].off0 = ((uint64_t)&base) & 0xFFFF; \
	idt[i].segsel = IDT_KCODESEG; \
	idt[i].ist = is; \
	idt[i].type = IDT_TYPE_INT; \
	idt[i].dpl = IDT_KDPL; \
	idt[i].present = IDT_PRESENT; \
	idt[i].off1 = (((uint64_t)&base) >> 16) & 0xFFFF; \
	idt[i].off2 = (uint32_t)(((uint64_t)&base) >> 32);

#define FREE_ISRS_START 	0x20
#define FREE_ISRS_COUNT		0xDF

struct ISR* idt_isrs;
uint8_t next_isr_hint;

void idt_init() {
	idt_isrs = kcalloc(sizeof(struct ISR), FREE_ISRS_COUNT);
	/* offset for easy ving, code will never access invalid index */

	/* mark 0x80 as used for syscalls*/
	idt_isrs[0x80 - FREE_ISRS_START].used = 1;

	next_isr_hint = 0;
}

uint8_t idt_claimIsrVector(uint64_t code) {
	const uint8_t start = next_isr_hint;
	do {
		if (idt_isrs[next_isr_hint].used == 0) {
			idt_isrs[next_isr_hint].used = 1;
			idt_isrs[next_isr_hint].code = code;
			return next_isr_hint + FREE_ISRS_START;
		}

		next_isr_hint++;
		if (next_isr_hint > FREE_ISRS_COUNT) {
			next_isr_hint = 0;
		}
	} while (start != next_isr_hint);

	/* out of ISRs */
	return 0;
}

void idt_freeIsrVector(uint8_t isr) {
	idt_isrs[isr - FREE_ISRS_START].used = 0;
}

uint64_t idt_translateCode(uint64_t code) {
	return idt_isrs[(uint8_t)code].code;
}

void idt_installisrs(volatile struct IDTD* idt, uint64_t* gdt, uint64_t* rsp) {
	/* first create TSS so that ISTs work */
	volatile struct TSS* tss  = (volatile struct TSS* )kmalloc(sizeof(struct TSS));
	volatile struct TSSD* tssd = (volatile struct TSSD* )&gdt[8];

	tssd->lim0 = (sizeof(struct TSS) - 1) & 0xFFFF;
	tssd->base0 = (uint64_t)tss & 0xFFFF;
	tssd->base1 = ((uint64_t)tss >> 16) & 0xFF;
	tssd->type = TYPE_TSS;
	tssd->dpl = IDT_UDPL;
	tssd->present = IDT_PRESENT;
	tssd->lim1 = ((sizeof(struct TSS) - 1) >> 16) & 0xF;
	tssd->gran = GRAN_BYTE;
	tssd->base2 = ((uint64_t)tss >> 24) & 0xFF;
	tssd->base3 = (uint32_t)((uint64_t)tss >> 32);

	tss->rsp00 = (uint32_t)(rsp[0]) & 0xFFFFFFFF;
	tss->rsp01 = (uint32_t)(rsp[0] >> 32) & 0xFFFFFFFF;

	// TODO: add rings 1 and 2

	tss->ist10 = (uint32_t)(rsp[1]) & 0xFFFFFFFF;
	tss->ist11 = (uint32_t)(rsp[1] >> 32) & 0xFFFFFFFF;

	tss->ist20 = (uint32_t)(rsp[2]) & 0xFFFFFFFF;
	tss->ist21 = (uint32_t)(rsp[2]> 32) & 0xFFFFFFFF;

	tss->ist30 = (uint32_t)(rsp[3]) & 0xFFFFFFFF;
	tss->ist31 = (uint32_t)(rsp[3]> 32) & 0xFFFFFFFF;

	tss->ist40 = (uint32_t)(rsp[4]) & 0xFFFFFFFF;
	tss->ist41 = (uint32_t)(rsp[4]> 32) & 0xFFFFFFFF;

	tss->ist50 = (uint32_t)(rsp[5]) & 0xFFFFFFFF;
	tss->ist51 = (uint32_t)(rsp[5]> 32) & 0xFFFFFFFF;

	tss->ist60 = (uint32_t)(rsp[6]) & 0xFFFFFFFF;
	tss->ist61 = (uint32_t)(rsp[6]> 32) & 0xFFFFFFFF;

	tss->ist70 = (uint32_t)(rsp[7]) & 0xFFFFFFFF;
	tss->ist71 = (uint32_t)(rsp[7]> 32) & 0xFFFFFFFF;

	// don't use iomap
	tss->iomap_addr = sizeof(struct TSS);

	ltr();

	IDTDE(ISR_DE, 0x00);
	IDTDE(ISR_DB, 0x01);	
	IDTDE(ISR_BP, 0x03);	
	IDTDE(ISR_OF, 0x04);	
	IDTDE(ISR_BR, 0x05);	
	IDTDE(ISR_UD, 0x06); 
	IDTDE(ISR_NM, 0x07); 
	IDTDE(ISR_TS, 0x0A); 
	IDTDE(ISR_NP, 0x0B);	
	IDTDE(ISR_SS, 0x0C);	
	IDTDE(ISR_GP, 0x0D);	
	IDTDE(ISR_PF, 0x0E);	
	IDTDE(ISR_MF, 0x10);	
	IDTDE(ISR_AC, 0x11);	
	IDTDE(ISR_XM, 0x13);	
	IDTDE(ISR_VE, 0x14);	
	IDTDE(ISR_CP, 0x15);	

	IDTDE2(ISR_NMI, 0x02, 1);	
	IDTDE2(ISR_DF, 0x08, 2);	
	IDTDE2(ISR_MC, 0x12, 3);	
}

void idt_installisr(volatile struct IDTD* idt, uint8_t index, uint8_t ist, uint8_t type, uint8_t dpl, uint8_t present) {
	const uint64_t irq_size = (uint64_t)&irq_dummy_end - (uint64_t)&irq_dummy_start;
	const uint64_t offsymb = (uint64_t)&irq_handlers + irq_size * ((uint64_t)index - FREE_ISRS_START);
	idt[index].off0 = offsymb & 0xFFFF;
	idt[index].segsel = IDT_KCODESEG;
	idt[index].ist = (uint8_t)(ist & 0x7);
	idt[index].type = (uint8_t)(type & 0xF);
	idt[index].dpl = (uint8_t)(dpl & 0x3);
	idt[index].present = (uint8_t)(present & 0x1);
	idt[index].off1 = (offsymb >> 16) & 0xFFFF;
	idt[index].off2 = (uint32_t)(offsymb >> 32);
}

void idt_installcustomisr(volatile struct IDTD* idt, uint64_t offsymb, uint8_t ist, uint8_t type, uint8_t dpl, uint8_t present, uint8_t v) {
	idt[v].off0 = offsymb & 0xFFFF;
	idt[v].segsel = IDT_KCODESEG;
	idt[v].ist = (uint8_t)(ist & 0x7);
	idt[v].type = (uint8_t)(type & 0xF);
	idt[v].dpl = (uint8_t)(dpl & 0x3);
	idt[v].present = (uint8_t)(present & 0x1);
	idt[v].off1 = (offsymb >> 16) & 0xFFFF;
	idt[v].off2 = (uint32_t)(offsymb >> 32);
}

#endif /* CORE_IDT_C */

