/* IDT.h - IDT structure */
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


#ifndef CORE_IDT_H
#define CORE_IDT_H

#define IDT_TYPE_INT		0xE
#define IDT_TYPE_TRAP		0xF

#define IDT_KCODESEG		0x8
#define IDT_KDPL				0x0
#define IDT_UDPL				0x3

#define IDT_PRESENT			0x1

extern uint64_t IDT_BASE;
extern uint64_t TSS_SEG;

extern uint64_t ISR_DE;
extern uint64_t ISR_DB;	
extern uint64_t ISR_BP;	
extern uint64_t ISR_OF;	
extern uint64_t ISR_BR;	
extern uint64_t ISR_UD; 
extern uint64_t ISR_NM; 
extern uint64_t ISR_TS; 
extern uint64_t ISR_NP;	
extern uint64_t ISR_SS;	
extern uint64_t ISR_GP;	
extern uint64_t ISR_PF;	
extern uint64_t ISR_MF;	
extern uint64_t ISR_AC;	
extern uint64_t ISR_XM;	
extern uint64_t ISR_VE;	
extern uint64_t ISR_CP;	

extern uint64_t ISR_NMI;
extern uint64_t ISR_DF;
extern uint64_t ISR_MC;

extern uint64_t rsp0;
extern uint64_t rsp1;
extern uint64_t rsp2;
extern uint64_t rsp3;
extern uint64_t rsp4;
extern uint64_t rsp5;
extern uint64_t rsp6;
extern uint64_t rsp7;

struct IDTD {
	uint16_t off0;
	uint16_t segsel;
	uint8_t ist : 3;
	uint8_t resv0 : 5;
	uint8_t type : 4;
	uint8_t resv1 : 1;
	uint8_t dpl : 2;
	uint8_t present : 1;
	uint16_t off1;
	uint32_t off2;
	uint32_t resv2;
} __attribute__((packed));

struct TSS {
	uint32_t resv0;

	uint32_t rsp00;
	uint32_t rsp01;
	uint32_t rsp10;
	uint32_t rsp11;
	uint32_t rsp20;
	uint32_t rsp21;

	uint32_t resv1;
	uint32_t resv2;

	uint32_t ist10;
	uint32_t ist11;
	uint32_t ist20;
	uint32_t ist21;
	uint32_t ist30;
	uint32_t ist31;
	uint32_t ist40;
	uint32_t ist41;
	uint32_t ist50;
	uint32_t ist51;
	uint32_t ist60;
	uint32_t ist61;
	uint32_t ist70;
	uint32_t ist71;

	uint32_t resv3;
	uint32_t resv4;
	uint16_t resv5;

	uint16_t iomap_addr;
} __attribute__((packed));

struct TSSD {
	uint16_t lim0;
	uint16_t base0;
	uint8_t base1;
	uint8_t type : 4;
	uint8_t resv0 : 1;
	uint8_t dpl : 2;
	uint8_t present : 1;
	uint8_t lim1 : 4;
	uint8_t avl : 1;
	uint8_t resv1 : 2;
	uint8_t gran : 1;
	uint8_t base2;
	uint32_t base3;
	uint32_t resv2;
} __attribute__((packed));

void idt_init(void);

uint16_t idt_getIsrVector(void);

void loadidt(void);

void idt_installisrs(struct IDTD* volatile idt, uint64_t* gdt, uint64_t* rsp);

void idt_installisr(struct IDTD* volatile idt, uint64_t offsymb, uint8_t ist, uint8_t type, uint8_t dpl, uint8_t present, uint8_t v);

#endif /* CORE_IDT_H */


