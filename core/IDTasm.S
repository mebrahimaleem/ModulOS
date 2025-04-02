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

	.code64
	.text
/* void loadidt(void); */
	.globl loadidt
loadidt:
	// set ist msr
	movq $SSP_table, %rdx
	movl %edx, %eax
	shrq $32, %rdx
	movl ISR_MSR, %ecx
	wrmsr

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

	mov $0x10, %ax
	mov %ax, %ss
	
	jmp ISR_EX_G

	.data

.align 8
SSP_table:
	.quad 0 // 0
	.quad rsp1 // IST1
	.quad rsp2
	.quad rsp3
	.quad rsp4
	.quad rsp5
	.quad rsp6
	.quad rsp7 // IST7

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
.rept 0x10
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

