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

#define LOADING_BOOTSTRAP		$0x1
#define LOADING_IDLE				$0x2

#define CMOS_NMI						$0x70
#define NMI_DISABLE					$0x80

	.section .text.real
gdt:
	gdt.nullseg:
		.quad 0
	gdt.codeseg:
		.quad 0x00a09a000000ffff
	gdt.dataseg:
		.quad 0x00a092000000ffff
	gdt.nullseg2:
		.quad 0
	gdt.code32seg:
		.quad 0xCF9A000000FFFF
	gdt.data32seg:
		.quad 0xCF92000000FFFF
	gdt.code16seg:
		.quad 0x9A000000FFFF
	gdt.data16seg:
		.quad 0x92000000FFFF
	.globl TSS_SEG
TSS_SEG:
	gdt.tssseg:
		.quad 0
		.quad 0
	gdt.usercodeseg:
		.quad 0 //TODO
	gdt.userdataseg:
		.quad 0 //TODO
	gdt.nullseg3:
		.quad 0
		.quad 0
		.quad 0
		.quad 0
	gdt.end:

	.globl lowmementer
lowmementer:
	.code16
	mov $0x38, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

	// set idt to bios ivt
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
	.long 0

	.code16
.section .text.mpboot
	.global mp_bootstrap
mp_bootstrap:
	jmp $0x0, $(bootstrap)

	.globl mp_loading
mp_loading:
	.byte 0

	.globl mp_rsp
mp_rsp:
	.quad 0

	.globl mp_cr3
mp_cr3:
	.quad 0

mp_gdtptr0:
	.short 0x7F
	.long 0x7000

	.globl mp_gdtptr
mp_gdtptr:
	.short 0x7F
	.quad 0

bootstrap:
	mov $0, %ax
	mov %ax, %ds

	movb LOADING_BOOTSTRAP, %al
	movb %al, mp_loading

	cli	

	/* disable NMIs */
	movb NMI_DISABLE, %al
	outb %al, CMOS_NMI
	
	/* Enable PAE and PSE */
	movl %cr4, %eax
	orl $0x30, %eax
	mov %eax, %cr4

	/* First use old PML4T for bootstrapping */
	movl $PML4T, %edi
	movl %edi, %cr3

	/* Enter compat mode */
	movl $0xC0000080, %ecx /* EFER */
	rdmsr
	orl $0x100, %eax /* LME */
	wrmsr

	movl %cr0, %ebx
	orl $0x80000001, %ebx
	movl %ebx, %cr0

	/* Enter 64-bit mode */
	lgdt mp_gdtptr0

	push $0
	push $0x8
	push $0
	push $mp_enter64
	retfl

	.code64
mp_enter64:
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

	/* now switch to the correct pml4t */
	movq mp_cr3, %rax
	movq %rax, %cr3

	/* now switch to correct gdt */
	lgdt mp_gdtptr

	/* set rsp */
	movq mp_rsp, %rsp

	/* reset eflags */
	pushq $0
	popfq

	//TODO: jump to another bootstrap routine in high memory

	movb LOADING_IDLE, %al
	movb %al, mp_loading
bootstrap.hang:
	jmp bootstrap.hang

#endif /* MULTIBOOT_REALMODE_S */
