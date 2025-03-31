/* ktransfer.S - transfer from low memory to high kernel */
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


#ifndef CORE_KTRANSFER_S
#define CORE_KTRANSFER_S

	.code64
	.text

	.globl ktransfer
ktransfer:
	/* offset esp */
	movq $(-0x80000000), %rax
	addq %rax, %rsp

	/* set gdt to high memory */
	lgdt gdt_ptr_high

	/* call kernel */
	popq %rsi
	popq %rdi

	call kentry

	/* fallback halt */
fallback_loop:
	hlt
	jmp fallback_loop

	.data

gdt_ptr_high:
	.short	0x3F
	.quad		-0x80000000 + 0x7000

#endif /* CORE_KTRANSFER_S */
