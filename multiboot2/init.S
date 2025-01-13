/* init.S - kernel init code */
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

#ifndef MULTIBOOT_INIT_S
#define MULTIBOOT_INIT_S

	.code32
	.text

	.globl pm2long
pm2long:
	pushl %ebx
	pushl %eax /* later passed to kernel */

	movl $msg_loading, %esi
	call pm_print
	
	/* Check if hardware is supported by OS */

	/* Check for CPUID */
	pushfl
	xorl $0x200000, (%esp)
	movl (%esp), %eax
	popfl
	pushfl
	xorl (%esp), %eax
	andl $0x200000, %eax
	jnz nosupport
	addl $4, %esp

	/* Check for long mode */
	movl $0x80000000, %eax
	cpuid
	cmpl $0x80000001, %eax
	jb nosupport

	movl $0x80000001, %eax
	cpuid
	test $0x20000000, %edx
	jz nosupport

	/* A20 line already enabled by grub */

	/* Identity map first four mebibytes */
	
	movl $PML4T, %edi
	movl %edi, %cr3

	movl $(PDPT + 3), (%edi)
	
	movl $PDPT, %edi
	movl $(PDT + 3), (%edi)

	movl $PDT, %edi
	movl $(0x83), (%edi) /* First two mebibytes */

	movl $(PDT + 8), %edi
	movl $(0x200000 + 0x83), (%edi) /* Second two mebibytes */

	/* Enable PAE and PSE */

	movl %cr4, %eax
	orl $0x30, %eax
	mov %eax, %cr4

	/* Enter compat mode */

	movl $0xC0000080, %ecx
	rdmsr
	orl $0x100, %eax
	wrmsr

	movl %cr0, %eax
	orl $(1 << 31), %eax
	movl %eax, %cr0

	/* Enter 64-bit mode */
	lgdt gdt_ptr
	
	.code64

	push $(gdt.codeseg - gdt)
	push $enter64
	retfq

enter64:
	mov $(gdt.dataseg - gdt), %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

	/* TODO: call kernel */

	/* fallback halt */
loop:
	hlt
	jmp loop

	.code32
nosupport:
	movl $msg_nosupport, %esi
	call pm_print
	jmp	loop

pm_print: // pm_print, esi points to string, modifies esi
	push %eax
	push %ebx

	movl (vga_offset), %eax

pm_print.loop:
		movb (%esi), %bl
		testb %bl, %bl
		jz pm_print.done
		movb %bl, (%eax)
		incl %eax

		movb $0x0F, (%eax)
		incl %eax
		incl %esi
		jmp pm_print.loop

pm_print.done:
	
	movl %eax, (vga_offset)

	popl %ebx
	popl %eax
	ret

	.data

msg_loading:
	.asciz "Loading..."

msg_nosupport:
	.asciz "FATAL: ModulOS does not support detected hardware!"

vga_offset:
	.long 0xb8000

	.align 8
gdt:
	gdt.nullseg:
		.quad 0
	gdt.codeseg:
		.quad	0x00a09a000000ffff
	gdt.dataseg:
		.quad	0x00a092000000ffff
	gdt.tssseg:
		.quad		0 /* TODO */
	gdt.end:

gdt_ptr:
	.short		gdt.end - gdt - 1
	.quad			gdt

	.bss

/* Paging Structures */

	.align 0x1000

	.comm PML4T, 0x1000
	.comm PDPT, 0x1000
	.comm PDT, 0x1000

#endif /* MULTIBOOT_INIT_S */
