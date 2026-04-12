/* isr.S - APIC ISRs */
/* Copyright (C) 2025-2026  Ebrahim Aleem
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

.globl apic_isr_timer
apic_isr_timer:
pushq %rax
pushq %rbx
pushq %rcx
pushq %rdx
pushq %rsi
pushq %rdi
pushq %r8
pushq %r9
pushq %r10
pushq %r11
pushq %r12
pushq %r13
pushq %r14
pushq %r15
pushq %rbp
movq %rsp, %rdi
jmp process_preempt_entry

.globl apic_isr_tlb_shootdown
apic_isr_tlb_shootdown:
pushq %rax
pushq %rcx
pushq %rdx
pushq %rsi
pushq %rdi
pushq %r8
pushq %r9
pushq %r10
pushq %r11
pushq %rbp

movq %rsp, %rbp
movq %rsp, %rax
andq $0xF, %rax
jz .aligned_tlb
subq %rax, %rsp
.aligned_tlb:
call apic_tlb_shootdown_dispatch
movq %rbp, %rsp

popq %rbp
popq %r11
popq %r10
popq %r9
popq %r8
popq %rdi
popq %rsi
popq %rdx
popq %rcx
popq %rax
iretq

.globl apic_isr_error
apic_isr_error:
pushq %rax
pushq %rcx
pushq %rdx
pushq %rsi
pushq %rdi
pushq %r8
pushq %r9
pushq %r10
pushq %r11
pushq %rbp

movq %rsp, %rbp
movq %rsp, %rax
andq $0xF, %rax
jz .aligned_error
subq %rax, %rsp
.aligned_error:
call apic_error_dispatch
movq %rbp, %rsp

popq %rbp
popq %r11
popq %r10
popq %r9
popq %r8
popq %rdi
popq %rsi
popq %rdx
popq %rcx
popq %rax
iretq
