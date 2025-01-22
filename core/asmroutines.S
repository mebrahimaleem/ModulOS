/* asmroutines.S - assembly routines */
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


#ifndef CORE_ASMROUTINES_S
#define CORE_ASMROUTINES_S

	.code64
	.text

/* void ktlbflush_addr(uint64_t addr); */
	.globl ktlbflush_addr
ktlbflush_addr:
	invlpg (%rdi)
	ret

/* void ktlb_flush(void); */
	.globl ktlbflush
ktlbflush:
	movq %cr3, %rax
	movq %rax, %cr3
	ret

/* __noreturn__ void kpanic_hlt(void); */
	.globl kpanic_hlt
kpanic_hlt:
	hlt
	jmp kpanic_hlt

	.bss

/* 32 mibibyte granularity so 256TiB * 1024 * 1024 / 32 / 8 = 1MiB bitmap*/
	.align 8
	.globl kpmembitmap
kpmembitmap:
.rept 0x20000
	.quad 0
.endr

#endif /* CORE_ASMROUTINES_S */
