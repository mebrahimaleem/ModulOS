/* process_control.S - kernel low level process control routines */
/* Copyright (C) 2026  Ebrahim Aleem
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

.globl process_resume
process_resume:
movq 0x98(%rdi), %rax
pushq %rax // ss
movq 0x00(%rdi), %rax
pushq %rax // rsp
movq 0x90(%rdi), %rax
pushq %rax // rflags
movq 0x88(%rdi), %rax
pushq %rax // cs
movq 0x80(%rdi), %rax
pushq %rax // rip

movq 0x08(%rdi), %rbp
movq 0x10(%rdi), %r15
movq 0x18(%rdi), %r14
movq 0x20(%rdi), %r13
movq 0x28(%rdi), %r12
movq 0x30(%rdi), %r11
movq 0x38(%rdi), %r10
movq 0x40(%rdi), %r9
movq 0x48(%rdi), %r8
movq 0x58(%rdi), %rsi
movq 0x60(%rdi), %rdx
movq 0x68(%rdi), %rcx
movq 0x70(%rdi), %rbx
movq 0x78(%rdi), %rax

movq 0x50(%rdi), %rdi

iretq
