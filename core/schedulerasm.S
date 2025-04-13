/* schedulerasm.S - scheduler routines */
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

#ifndef CORE_SCHEDULERASM_S
#define CORE_SCHEDULERASM_S

#define RAX_OFF	0x00
#define RBX_OFF	0x08
#define RCX_OFF	0x10
#define RDX_OFF	0x18
#define RSP_OFF	0x20
#define RBP_OFF	0x28
#define RSI_OFF	0x30
#define RDI_OFF	0x38
#define R8_OFF	0x40
#define R9_OFF	0x48
#define R10_OFF	0x50
#define R11_OFF	0x58
#define R12_OFF	0x60
#define R13_OFF	0x68
#define R14_OFF	0x70
#define R15_OFF	0x78

#define DS_OFF	0x80
#define FS_OFF	0x88
#define CS_OFF	0x90
#define CR3_OFF	0x98

#define RFL_OFF	0xA0
#define RIP_OFF	0xA8

#define FRAME_RSP_OFF	0xC8
#define FRAME_CS_OFF	0xB8
#define FRAME_RFL_OFF	0xC0
#define FRAME_RIP_OFF	0xB0

#define FS_MSR	0xC0000100

/* __attribute__((noreturn)) void scheduler_transferTo(struct PCB* pcb); */
	.globl scheduler_transferTo
scheduler_transferTo:
	/* set general purpose registers */

	/* skip rax */
	movq RBX_OFF(%rdi), %rbx
	movq RCX_OFF(%rdi), %rcx
	movq RDX_OFF(%rdi), %rdx
	/* skip rsp*/
	movq RBP_OFF(%rdi), %rbp
	movq RSI_OFF(%rdi), %rsi
	/* skip rdi */
	movq R8_OFF(%rdi), %r8
	movq R9_OFF(%rdi), %r9
	movq R10_OFF(%rdi), %r10
	movq R11_OFF(%rdi), %r11
	movq R12_OFF(%rdi), %r12
	movq R13_OFF(%rdi), %r13
	movq R14_OFF(%rdi), %r14
	movq R15_OFF(%rdi), %r15

	/* align stack to 16 bytes */
	subq $0x10, %rsp
	andq $~0xF, %rsp

	/* set cr3 */
	movq CR3_OFF(%rdi), %rax
	movq %rax, %cr3

	/* TODO: implement fs */

	/* set data segment selectors */
	movq DS_OFF(%rdi), %rax
	movw %ax, %ds
	/* skip ss */
	movw %ax, %es
	movw %ax, %gs

	/* set iret frame for context switch */
	pushq %rax /* ss */

	movq RSP_OFF(%rdi), %rax
	pushq %rax /* rsp */

	movq RFL_OFF(%rdi), %rax
	pushq %rax /* rfl */

	movq CS_OFF(%rdi), %rax
	pushq %rax /* cs */

	movq RIP_OFF(%rdi), %rax
	pushq %rax /* rip */

	/* rax and rdi */
	movq RAX_OFF(%rdi), %rax
	movq RDI_OFF(%rdi), %rdi

	/* iret to new context */
	iretq

	.globl scheduler_isr
scheduler_isr:
	subq $8, %rsp /* skip rip */
	subq $8, %rsp /* skip rfl */

	subq $8, %rsp /* skip cr3 */

	subq $8, %rsp /* skip cs */

	subq $8, %rsp /* skip fs, TODO */

	subq $8, %rsp /* skip ds */

	pushq %r15
	pushq %r14
	pushq %r13
	pushq %r12
	pushq %r11
	pushq %r10
	pushq %r9
	pushq %r8
	pushq %rdi
	pushq %rsi
	pushq %rbp
	subq $8, %rsp /* skip rsp */
	pushq %rdx
	pushq %rcx
	pushq %rbx
	pushq %rax

	movq %cr3, %rax
	movq %rax, CR3_OFF(%rsp) /* cr3 */

	movq %ds, %rax
	movq %rax, DS_OFF(%rsp) /* ds */

	movq FRAME_RSP_OFF(%rsp), %rax
	movq %rax, RSP_OFF(%rsp) /* rsp */

	movq FRAME_CS_OFF(%rsp), %rax
	movq %rax, CS_OFF(%rsp) /* cs */

	movq FRAME_RFL_OFF(%rsp), %rax
	movq %rax, RFL_OFF(%rsp) /* rfl */

	movq FRAME_RIP_OFF(%rsp), %rax
	movq %rax, RIP_OFF(%rsp) /* rip */

	/* TODO: restore cr3 */

	movq %rsp, %rdi

	call scheduler_reenter

#endif /* CORE_SCHEDULERASM_S*/


