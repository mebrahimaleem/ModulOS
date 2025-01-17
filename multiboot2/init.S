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

#ifndef MULTIBOOT2_INIT_S
#define MULTIBOOT2_INIT_S

	.code32
	.section .text.boot

	.globl pm2long
pm2long:
	pushl $0
	pushl %ebx
	pushl $0
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

	/* Check cpuid */
	movl $1, %eax
	cpuid
	and $((1 << 3) /* PSE */ + (1 << 5) /* MSR */ + (1 << 6) /* PAE */ + (1 << 9) /* APIC */), %edx
	cmp $((1 << 3) /* PSE */ + (1 << 5) /* MSR */ + (1 << 6) /* PAE */ + (1 << 9) /* APIC */), %edx
	jne nosupport

	/* Check for long mode */
	movl $0x80000000, %eax /* check cpuid extensions long mode */
	cpuid
	cmpl $0x80000001, %eax
	jb nosupport

	movl $0x80000001, %eax /* check long mode */
	cpuid
	test $0x20000000, %edx
	jz nosupport

	/* reenter real mode to set video mode */

	lgdt gdt_ptr32

	/* setup up the stackframe for returning */
	push $enter32

	/* setup stackframe for realmode */
	push $0x30
	push $0x7040
	retfl

	.code32
enter32:
	/* Identity map first four mibibytes*/
	
	movl $PML4T, %edi
	movl %edi, %cr3

	movl $(PDPT0 + 3), (%edi)

	movl $(PML4T + 0x1000 - 8), %edi
	movl $(PDPT1 + 3), (%edi)
	
	movl $PDPT0, %edi
	movl $(PDT + 3), (%edi)

	movl $(PDT), %edi
	movl $(0x83), (%edi) /* First two mibibytes */

	movl $(PDT + 8), %edi
	movl $(0x200000 + 0x83), (%edi) /* Second two mibibytes */

	/* Map last two gibibytes to first two */

	movl $(PDPT1 + 0x1000 - 16), %edi
	movl $(0x83), (%edi) /* First gibibytes */

	movl $(PDPT1 + 0x1000 - 8), %edi
	movl $(0x200000 + 0x83), (%edi) /* Second  gibibyte */

	/* Enable PAE and PSE */

	movl %cr4, %eax
	orl $0x30, %eax
	mov %eax, %cr4

	/* Enter compat mode */

	movl $0xC0000080, %ecx /* EFER */
	rdmsr
	orl $0x100, %eax /* LME */
	wrmsr

	movl %cr0, %eax
	orl $(1 << 31), %eax
	movl %eax, %cr0

	/* Enter 64-bit mode */
	lgdt gdt_ptr

	push $0x8
	push $enter64
	retfl

	.code64
enter64:
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

	/* clear screen */

	movq $8000, %rcx
	movq $0xa0000, %rdi
	movq $0x101010101010101, %rax
	rep stosq

	/* far return to ktransfer */
	push $0x8
	push $ktransfer
	retfq

hang_loop:
	hlt
	jmp hang_loop

	.code32
nosupport:
	movl $msg_nosupport, %esi
	call pm_print
	jmp	hang_loop

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

	.section .data.boot

msg_loading:
	.asciz "Loading..."

msg_nosupport:
	.asciz "FATAL: ModulOS does not support detected hardware!"

vga_offset:
	.long 0xb8000

/* GDT */

gdt_ptr:
	.short 0x3F
	.quad 0x7000

gdt_ptr32:
	.short 0x3F
	.long 0x7000

/* Paging Structures */

	.align 0x1000

PML4T:
.rept 0x200
	.quad 0
.endr

PDPT0:
.rept 0x200
	.quad 0
.endr

PDPT1:
.rept 0x200
	.quad 0
.endr

PDT:
.rept 0x200
	.quad 0
.endr

	.section .bss.boot

#endif /* MULTIBOOT2_INIT_S */
