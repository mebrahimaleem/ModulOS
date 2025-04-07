/* lapic.h - Local APIC routines */
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

#ifndef APIC_LAPIC_H
#define APIC_LAPIC_H

#define LAPIC_DLVRY_FIXED			0x0
#define LAPIC_DLVRY_NMI				0x4
#define LAPIC_DLVRY_SMI				0x2
#define LAPIC_DLVRY_INIT			0x5
#define LAPIC_DLVRY_EXTINT		0x7
#define LAPIC_DLVRY_STARTUP		0x6

#define LAPIC_TRIGMODE_EDGE		0x0
#define LAPIC_TRIGMODE_LVL		0x1

#define LAPIC_SPUR_V					0xFF

#define LAPIC_IPI_FIXED				0x000
#define LAPIC_IPI_LP					0x100
#define LAPIC_IPI_SMI					0x200
#define LAPIC_IPI_NMI					0x400
#define LAPIC_IPI_INIT				0x500
#define LAPIC_IPI_STARTUP			0x600

#define LAPIC_IPI_PHYSC				0x000
#define LAPIC_IPI_LOGIC				0x800

#define LAPIC_IPI_ASSERT			0x0000
#define LAPIC_IPI_DEASSERT		0x4000

#define LAPIC_IPI_EDGE				0x0000
#define LAPIC_IPI_LEVL				0x8000

#define LAPIC_IPI_NOSHORT			0x00000
#define LAPIC_IPI_SELF				0x40000
#define LAPIC_IPI_ALL					0x80000
#define LAPIC_IPI_ALLEX				0xC0000

#define LAPIC_CMCI_CODE		0x0
#define LAPIC_TIMR_CODE		0x1
#define LAPIC_THRM_CODE		0x2
#define LAPIC_PRCR_CODE		0x3
#define LAPIC_LNT0_CODE		0x4
#define LAPIC_EROR_CODE		0x6
#define LAPIC_SPURIOUS_VECTOR	0xFF

extern uint64_t ISR_CMCI;
extern uint64_t ISR_TIMR;
extern uint64_t ISR_THRM;
extern uint64_t ISR_PRCR;
extern uint64_t ISR_LNT0;
extern uint64_t ISR_LNT1;
extern uint64_t ISR_EROR;

extern uint64_t ISR_SPUR;

union LAPIC_LVT {
	uint32_t zero;

	struct {
		uint32_t vector : 8;
		uint32_t resv0 : 4;
		uint32_t dlvry_sts : 1;
		uint32_t resv1 : 3;
		uint32_t mask : 1;
		uint32_t timr_mode : 2;
		uint32_t resv2 : 13;
	} timer __attribute__((packed));

	struct {
		uint32_t vector : 8;
		uint32_t dlvry_mode : 3;
		uint32_t resv0 : 1;
		uint32_t dlvry_sts: 1;
		uint32_t resv1 : 3;
		uint32_t mask : 1;
		uint32_t resv2 : 15;
	} cmci __attribute__((packed));

	struct {
		uint32_t vector : 8;
		uint32_t dlvry_mode : 3;
		uint32_t resv0 : 1;
		uint32_t dlvry_sts: 1;
		uint32_t iipp : 1;
		uint32_t rirr : 1;
		uint32_t trig_mode : 1;
		uint32_t mask : 1;
		uint32_t resv2 : 15;
	} lint0 __attribute__((packed));

	struct {
		uint32_t vector : 8;
		uint32_t dlvry_mode : 3;
		uint32_t resv0 : 1;
		uint32_t dlvry_sts: 1;
		uint32_t iipp : 1;
		uint32_t rirr : 1;
		uint32_t trig_mode : 1;
		uint32_t mask : 1;
		uint32_t resv2 : 15;
	} lint1 __attribute__((packed));

	struct {
		uint32_t vector : 8;
		uint32_t resv0 : 4;
		uint32_t dlvry_sts: 1;
		uint32_t resv1 : 3;
		uint32_t mask : 1;
		uint32_t resv2 : 15;
	} error __attribute__((packed));

	struct {
		uint32_t vector : 8;
		uint32_t dlvry_mode : 3;
		uint32_t resv0 : 1;
		uint32_t dlvry_sts: 1;
		uint32_t resv1 : 3;
		uint32_t mask : 1;
		uint32_t resv2 : 15;
	} perfcm __attribute__((packed));

	struct {
		uint32_t vector : 8;
		uint32_t dlvry_mode : 3;
		uint32_t resv0 : 1;
		uint32_t dlvry_sts: 1;
		uint32_t resv1 : 3;
		uint32_t mask : 1;
		uint32_t resv2 : 15;
	} thrm __attribute__((packed));
};

void apic_initlocal(void);

void apic_initlocalap(uint64_t* idt);

void apic_lapic_sendipi(uint8_t v, uint32_t flg, uint8_t dest);

void apic_lapic_waitForIpi(void);

uint8_t apic_getId(void);

void apic_lapic_sendeoi(void);

#endif /* APIC_LAPIC_H */
