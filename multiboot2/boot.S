/* boot.S - multiboot2 entry point */
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

#define _ASM
#include <multiboot2/init.h>

#define MULTIBOOT2_MAGIC	0xE85250D6
#define MULTIBOOT2_CHECK	0x36D76289

#define ARCH_I386_PM			0

#define TYPE_INFO	1
#define TYPE_FRBF	5

#define MBITAG_NOTOPT	0

#define FRAMEBUFFER_WIDTH		1024
#define FRAMEBUFFER_HEIGHT	768
#define FRAMEBUFFER_DEPTH		32

#define STACK_SIZE				0x4000

#define NMI_PORT	0x80
#define NMI_MASK	0x70

#define	EFL_CPUIDBIT	0x200000

#define	CPUID_CHECK	1
#define CPUID_PSE		(1 << 3)
#define CPUID_MSR		(1 << 5)
#define CPUID_PAE		(1 << 6)
#define CPUID_APIC	(1 << 9)

#define CPUID_CHECK_LONGMODE	0x80000000
#define CPUID_LONGMODE				0x80000001
#define CPUID_LONGMODE_EXIST	0x20000000

#define PAEPSE	0x30

#define EFER	0xC0000080
#define LME		0x100
#define PG		(1 << 31)

#define GDT_RW	(1 << 1)
#define GDT_E		(1 << 3)
#define GDT_S		(1 << 4)
#define GDT_P		(1 << 7)
#define GDT_L		(1 << 5)
#define GDT_G		(1 << 7)

#define KERNEL_PAGE_FLAGS	(1 + 2)
#define PAGE_SZ						(1 << 7)

#define PAGING_1GIB	0x40000000

	.code32
	.section .text.multiboot2

	.align	8
multiboot2_header:
	/* magic */
	.long		MULTIBOOT2_MAGIC

	/* i386 arch */
	.long		ARCH_I386_PM

	/* length */
	.long		multiboot2_header_end - multiboot2_header

	/* checksum */
	.long		-(MULTIBOOT2_MAGIC + ARCH_I386_PM + (multiboot2_header_end - multiboot2_header))

	/* information request tag */
multiboot2_header.info_start:
	.short	TYPE_INFO
	.short	MBITAG_NOTOPT
	.long		multiboot2_header.info_end - multiboot2_header.info_start
	.long		MBITAG_TYPE_MEMMAP
#ifdef GRAPHICSBASE
	.long		MBITAG_TYPE_FRMBUF
#endif /* GRAPHICSBASE */
	.long		MBITAG_TYPE_RSDPV2
	.align 8, 0
multiboot2_header.info_end:

#ifdef GRAPHICSBASE
	/* frambuffer tag */
multiboot2_header.frbf_start:
	.short	TYPE_FRBF
	.short	MBITAG_NOTOPT
	.long		multiboot2_header.frbf_end - multiboot2_header.frbf_start
	.long		FRAMEBUFFER_WIDTH
	.long		FRAMEBUFFER_HEIGHT
	.long		FRAMEBUFFER_DEPTH
	.long		0
multiboot2_header.frbf_end:
#endif /* GRAPHICSBASE */

	/* terminate tags */
	.short	0
	.short	0
	.long		8
multiboot2_header_end:

	.section .text.boot
	.globl _start
_start:
	/* init stack */
	movl $(stack + STACK_SIZE), %esp
	movl %esp, %ebp

	/* eflags */
	pushl $0
	popfl

	/* verify correct bootloader */
	cmp $MULTIBOOT2_CHECK, %eax
	jne hang

	/* save ebx */
	pushl %ebx

	/* disable NMIs */
	movb $NMI_PORT, %al
	outb %al, $NMI_MASK

	/* Check if hardware is supported by OS */

	/* Check for CPUID */
	pushfl
	pushfl
	xorl $EFL_CPUIDBIT, (%esp)
	popfl
	pushfl
	popl %eax
	xorl (%esp), %eax
	testl %eax, %eax
	jz hang
	addl $4, %esp

	/* Check cpuid */
	movl $CPUID_CHECK, %eax
	cpuid
	and $(CPUID_PSE + CPUID_MSR + CPUID_PAE + CPUID_APIC), %edx
	cmp $(CPUID_PSE + CPUID_MSR + CPUID_PAE + CPUID_APIC), %edx
	jne hang

	/* Check for long mode */
	movl $CPUID_CHECK_LONGMODE, %eax /* check cpuid extensions long mode */
	cpuid
	cmpl $CPUID_LONGMODE, %eax
	jb hang

	movl $CPUID_LONGMODE, %eax /* check long mode */
	cpuid
	test $CPUID_LONGMODE_EXIST, %edx
	jz hang

	/* Setup paging */

	movl $PML4, %esi
	movl %esi, %cr3

	/* restore ebx */
	popl %ebx

	/* Enable PAE and PSE */

	movl %cr4, %eax
	orl $PAEPSE, %eax
	mov %eax, %cr4

	/* Enter compat mode */

	movl $EFER, %ecx /* EFER */
	rdmsr
	orl $LME, %eax /* LME */
	wrmsr

	movl %cr0, %eax
	orl $PG, %eax
	movl %eax, %cr0

	/* Enter 64-bit mode */
	lgdt gdt_ptr32

	pushl $CODE_SEG
	pushl $enter64
	retfl

hang:
	hlt
	jmp hang

	.code64
enter64:
	mov $DATA_SEG, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

	/* call kentry and pass rdi with multiboot2 info */
	xorq %rdi, %rdi
	movl %ebx, %edi

	/* far return to high memory */
	pushq $CODE_SEG
	pushq $ktransfer
	retfq

	.section .text
ktransfer:
	addq $KERNEL_VMA, gdt_ptr64.addr
	lgdt gdt_ptr64
	addq $KERNEL_VMA, %rsp
	call multiboot2_init
	call kentry

	.section .data.boot

	/* GDT */
	.align	8
gdt:
	gdt.nullseg:
		.quad 0
	gdt.codeseg:
		.short	0x0000	/* limit 0:15 */
		.short	0x0000	/* base 0:15 */
		.byte		0x00		/* base 16:23 */
		.byte		GDT_RW + GDT_E + GDT_S + GDT_P	/* access: RW E S P */
		.byte		GDT_L + GDT_G	/* limit 16:19 flag: L G */
		.byte		0x00		/* base 24:31 */
	gdt.dataseg:
		.short	0x0000	/* limit 0:15 */
		.short	0x0000	/* base 0:15 */
		.byte		0x00		/* base 16:23 */
		.byte		GDT_RW + GDT_S + GDT_P	/* access: RW S P */
		.byte		GDT_G		/* limit 16:19 flag: G */
		.byte		0x92		/* access: RW S P */
		.byte		0x80		/* limit 16:19 flag: G */
		.byte		0x00		/* base 24:31 */
	gdt.end:

	.equ	CODE_SEG, gdt.codeseg - gdt
	.equ	DATA_SEG, gdt.dataseg - gdt

gdt_ptr32:
	.short gdt.end - gdt - 1
	.long gdt

gdt_ptr64:
	.short gdt.end - gdt - 1
gdt_ptr64.addr:
	.quad gdt

	/* Paging structures */
	.align 0x1000

PML4:
	.quad PDPT_0 + KERNEL_PAGE_FLAGS
.rept 0x200 - 2
	.quad 0
.endr
	.quad PDPT_511 + KERNEL_PAGE_FLAGS

PDPT_0:
	.quad (0 * PAGING_1GIB) + KERNEL_PAGE_FLAGS + PAGE_SZ
	.quad (1 * PAGING_1GIB) + KERNEL_PAGE_FLAGS + PAGE_SZ
.rept 0x200 - 2
	.quad 0
.endr

PDPT_511:
.rept 0x200 - 2
	.quad 0
.endr
	.quad (0 * PAGING_1GIB) + KERNEL_PAGE_FLAGS + PAGE_SZ
	.quad (1 * PAGING_1GIB) + KERNEL_PAGE_FLAGS + PAGE_SZ

	.section .bss.boot
	.align 0x1000

	/* allocate stack in bss */
stack:
.rept STACK_SIZE
	.byte 0
.endr
