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

#define LAPIC_CMCI_V					0x20
#define LAPIC_TIMR_V					0x21
#define LAPIC_THRM_V					0x22
#define LAPIC_PRCR_V					0x23
#define LAPIC_LNT0_V					0x24
#define LAPIC_LNT1_V					0x25
#define LAPIC_EROR_V					0x26

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

#endif /* APIC_LAPIC_H */
