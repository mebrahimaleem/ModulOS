/* realmode.S - realmode code */
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

#ifndef MULTIBOOT_REALMODE_S
#define MULTIBOOT_REALMODE_S
	.section .text.gdt
gdt:
	gdt.nullseg:
		.quad 0
	gdt.codeseg:
		.quad 0x00a09a000000ffff
	gdt.dataseg:
		.quad 0x00a092000000ffff
	gdt.tssseg:
		.quad 0 /* TODO */
	gdt.code32seg:
		.quad 0xCF9A000000FFFF
	gdt.data32seg:
		.quad 0xCF92000000FFFF
	gdt.code16seg:
		.quad 0x9A000000FFFF
	gdt.data16seg:
		.quad 0x92000000FFFF
	gdt.end:

/* . = 0x7040 */
	.code16
	mov $0x38, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss
	
	lidt idt_ptr

	movl %cr0, %eax
	xorb $1, %al /* toggle (clear) PE */
	movl %eax, %cr0

	push $0
	push $enterreal
	retf

enterreal:
	mov $0x0, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss
	
	sti

	/* set vga to 320x200 256 color mode */

	xorb %ah, %ah
	movb $0x13, %al
	int $0x10 

	/* back to pm */
	cli
	movl %cr0, %eax
	orb $1, %al
	movl %eax, %cr0

	push $0x20
	push $enterpm
	retf

	.code32
enterpm:
	mov $0x28, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

	ret

idt_ptr:
	.short 0x2FF
	.long 0x0

#endif /* MULTIBOOT_REALMODE_S */
