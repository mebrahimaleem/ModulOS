/* exceptions.h - exceptions interrrupt service routines*/
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

.globl isr_00
isr_00:
pushq $0x00
pushq $0x00
jmp exception_generic

.globl isr_01
isr_01:
pushq $0x00
pushq $0x01
jmp exception_generic

.globl isr_02
isr_02:
pushq $0x00
pushq $0x02
jmp exception_generic

.globl isr_03
isr_03:
pushq $0x00
pushq $0x03
jmp exception_generic

.globl isr_04
isr_04:
pushq $0x00
pushq $0x04
jmp exception_generic

.globl isr_05
isr_05:
pushq $0x00
pushq $0x05
jmp exception_generic

.globl isr_06
isr_06:
pushq $0x00
pushq $0x06
jmp exception_generic

.globl isr_07
isr_07:
pushq $0x00
pushq $0x07
jmp exception_generic

.globl isr_08
isr_08:
pushq $0x08
jmp exception_generic

.globl isr_09
isr_09:
pushq $0x00
pushq $0x09
jmp exception_generic

.globl isr_0a
isr_0a:
pushq $0x0a
jmp exception_generic

.globl isr_0b
isr_0b:
pushq $0x0b
jmp exception_generic

.globl isr_0c
isr_0c:
pushq $0x0c
jmp exception_generic

.globl isr_0d
isr_0d:
pushq $0x0d
jmp exception_generic

.globl isr_0e
isr_0e:
pushq $0x0e
jmp exception_generic

.globl isr_10
isr_10:
pushq $0x00
pushq $0x10
jmp exception_generic

.globl isr_11
isr_11:
pushq $0x11
jmp exception_generic

.globl isr_12
isr_12:
pushq $0x00
pushq $0x12
jmp exception_generic

.globl isr_13
isr_13:
pushq $0x00
pushq $0x13
jmp exception_generic

.globl isr_14
isr_14:
pushq $0x00
pushq $0x14
jmp exception_generic

.globl isr_15
isr_15:
pushq $0x15
jmp exception_generic

.globl isr_1c
isr_1c:
pushq $0x00
pushq $0x1c
jmp exception_generic

.globl isr_1d
isr_1d:
pushq $0x1d
jmp exception_generic

.globl isr_1e
isr_1e:
pushq $0x1e
jmp exception_generic

exception_generic:
// iret frame already pushed
// code and vector already pushed
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
pushq %rsp
movq %rsp, %rdi
// align stack
movq %rsp, %rax
andq $0xF, %rax
jz exception_dispatch
subq %rax, %rsp
jmp exception_dispatch
