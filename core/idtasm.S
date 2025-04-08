/* IDTasm.S - IDT structure */
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


#ifndef CORE_IDTasm_S
#define CORE_IDTasm_S

#define ISR_MSR			$0x6A8

#define EX_DE	$0x00
#define EX_DB	$0x01
#define EX_BP	$0x03
#define EX_OF	$0x04
#define EX_BR	$0x05
#define EX_UD $0x06
#define EX_NM $0x07
#define EX_TS $0x0A
#define EX_NP	$0x0B
#define EX_SS	$0x0C
#define EX_GP	$0x0D
#define EX_PF	$0x0E
#define EX_MF	$0x10
#define EX_AC	$0x11
#define EX_XM	$0x13
#define EX_VE	$0x14
#define EX_CP	$0x15

#define EX_NMI	$0x02
#define EX_DF		$0x08
#define EX_MC		$0x12

#define LAPIC_SPUR_V					$0xFF

	.code64
	.text
/* void loadidt(void); */
	.globl loadidt
loadidt:
	lidt idt_ptr
		
	ret

// ISRs

ISR_EX_G:
	// pass error code
	movq %cr3, %rdx

	// setup for kernel
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

	movq $k0PML4T, %rax
	movq %rax, %cr3
	
	call exception_handler
	// iret will occur elsewhere

	.globl ISR_DE
ISR_DE:
	movq EX_DE, %rdi
	movq %rsp, %rsi
	jmp ISR_EX_G
	.globl ISR_DB
ISR_DB:
	movq EX_DB, %rdi
	movq %rsp, %rsi
	jmp ISR_EX_G
	.globl ISR_BP
ISR_BP:
	movq EX_BP, %rdi
	movq %rsp, %rsi
	jmp ISR_EX_G
	.globl ISR_OF
ISR_OF:
	movq EX_OF, %rdi
	movq %rsp, %rsi
	jmp ISR_EX_G
	.globl ISR_BR
ISR_BR:
	movq EX_BR, %rdi
	movq %rsp, %rsi
	jmp ISR_EX_G
	.globl ISR_UD
ISR_UD:
	movq EX_UD, %rdi
	movq %rsp, %rsi
	jmp ISR_EX_G
	.globl ISR_NM
ISR_NM:
	movq EX_NM, %rdi
	movq %rsp, %rsi
	jmp ISR_EX_G
	.globl ISR_TS
ISR_TS:
	movq EX_TS, %rdi
	movq %rsp, %rsi
	jmp ISR_EX_G
	.globl ISR_NP
ISR_NP:
	movq EX_NP, %rdi
	movq %rsp, %rsi
	jmp ISR_EX_G
	.globl ISR_SS
ISR_SS:
	movq EX_SS, %rdi
	movq %rsp, %rsi
	jmp ISR_EX_G
	.globl ISR_GP
ISR_GP:
	movq EX_GP, %rdi
	movq %rsp, %rsi
	jmp ISR_EX_G
	.globl ISR_PF
ISR_PF:
	movq EX_PF, %rdi
	movq %rsp, %rsi
	jmp ISR_EX_G
	.globl ISR_MF
ISR_MF:
	movq EX_MF, %rdi
	movq %rsp, %rsi
	jmp ISR_EX_G
	.globl ISR_AC
ISR_AC:
	movq EX_AC, %rdi
	movq %rsp, %rsi
	jmp ISR_EX_G
	.globl ISR_XM
ISR_XM:
	movq EX_XM, %rdi
	movq %rsp, %rsi
	jmp ISR_EX_G
	.globl ISR_VE
ISR_VE:
	movq EX_VE, %rdi
	movq %rsp, %rsi
	jmp ISR_EX_G
	.globl ISR_CP
ISR_CP:
	movq EX_CP, %rdi
	movq %rsp, %rsi
	jmp ISR_EX_G

	.globl ISR_NMI
ISR_NMI:
	movq EX_NMI, %rdi
	movq %rsp, %rsi
	jmp ISR_EX_G
	.globl ISR_DF
ISR_DF:
	movq EX_DF, %rdi
	movq %rsp, %rsi
	jmp ISR_EX_G
	.globl ISR_MC
ISR_MC:
	movq EX_MC, %rdi
	movq %rsp, %rsi
	jmp ISR_EX_G

ISR_G:
	// save registers
	pushq %rsi
	pushq %rax
	pushq %rcx
	pushq %rdx
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	// kernel does not use floating point, so no need to save them

	// save cr3
	movq %cr3, %rsi
	pushq %rsi //cr3

	// save data segment selectors
	mov %ds, %ax
	pushq %rax

	// setup for kernel
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

	movq $k0PML4T, %rax
	movq %rax, %cr3
	
	call isr_handler

	// eoi sent by handler
	// restore data segment selectors
	popq %rax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

	// restore paging
	popq %rsi
	movq %rsi, %cr3

	// restore registers
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rdx
	popq %rcx
	popq %rax
	popq %rsi
	popq %rdi
	iretq

	.globl irq_handlers
irq_handlers:
.macro ISR_STUB n
isr_handler_\n:
	pushq %rdi
	movq $\n, %rdi
	jmp ISR_G
/* pad to constant size for easy indexing */
	.skip (0x10 - (. - isr_handler_\n))
.endm


.irp i, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31, \
32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63, \
64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95, \
96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123, \
124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151, \
152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179, \
180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207, \
208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223
    ISR_STUB \i
.endr

	.globl irq_dummy_start
irq_dummy_start:
	ISR_STUB 0xFF
	.globl irq_dummy_end
irq_dummy_end:

	.data

idt_ptr:
	.short IDT_END - IDT_BASE - 1
	.quad IDT_BASE

	.bss
	
.align 8
	.globl IDT_BASE
IDT_BASE:
.rept 0x1000 // wil be populated at runtime
	.quad 0
.endr
IDT_END:

// stacks

// default kernel
.align 0x1000
	.globl rsp0
.rept 0x800
	.quad 0
.endr
rsp0:

// smaller ones for NMI, MC, and DF
	.globl rsp1
.rept 0x200
	.quad 0
.endr
rsp1:

	.globl rsp2
.rept 0x200
	.quad 0
.endr
rsp2:

	.globl rsp3
.rept 0x200
	.quad 0
.endr
rsp3:

// really small ones, probably not every needed
	.globl rsp4
.rept 0x10
	.quad 0
.endr
rsp4:

	.globl rsp5
.rept 0x10
	.quad 0
.endr
rsp5:

	.globl rsp6
.rept 0x10
	.quad 0
.endr
rsp6:

	.globl rsp7
.rept 0x10
	.quad 0
.endr
rsp7:

#endif /* CORE_IDTasm_S */

