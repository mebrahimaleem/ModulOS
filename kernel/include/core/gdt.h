/* gdt.h - global descriptor table interface */
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

#ifndef KERNEL_CORE_GDT_H
#define KERNEL_CORE_GDT_H

#include <stdint.h>

#define GDT_NUM_ENTRIES	6

#define GDT_ACS_TSS	0x09
#define GDT_ACS_P		0x80

#define GDT_LIMIT0_MASK	0x0FFFF
#define GDT_LIMIT1_MASK 0xF0000
#define GDT_LIMIT1_SHFT	16

#define GDT_BASE0_MASK	0x000000000000FFFF
#define GDT_BASE1_MASK	0x0000000000FF0000
#define GDT_BASE2_MASK	0x00000000FF000000
#define GDT_BASE3_MASK	0xFFFFFFFF00000000
#define GDT_BASE1_SHFT	16
#define GDT_BASE2_SHFT	24
#define GDT_BASE3_SHFT	32

#define GDT_CODE_INDEX	1
#define GDT_DATA_INDEX	2
#define GDT_UCODE_INDEX	3
#define GDT_UDATA_INDEX	4
#define GDT_TSS_INDEX		5

struct gdt_t {
	uint16_t ign0;
	uint16_t ign1;
	uint8_t ign2;
	uint8_t access;
	uint8_t flg;
	uint8_t ign3;
};

struct gdt_sys_t {
	uint16_t limit0;
	uint16_t base0;
	uint8_t base1;
	uint8_t access;
	uint8_t flglmt1;
	uint8_t base2;
	uint32_t base3;
	uint32_t resv;
} __attribute__((packed));

#endif /* KERNEL_CORE_GDT_H */
