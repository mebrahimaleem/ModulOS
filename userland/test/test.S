/* test.S - userland testing code */
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

#include <modulos/syscall_vectors.h>

.section .text

.globl _start
_start:

movq $SYSCALL_OPEN, %rax
leaq file, %rdi
syscall

movq %rax, %rbx

movq $SYSCALL_READ, %rax
movq %rbx, %rdi
leaq buffer, %rsi
movq $8, %rdx
syscall

movq $SYSCALL_CLOSE, %rax
movq %rbx, %rdi
syscall

movq $SYSCALL_EXIT, %rax
movq $0, %rdi
syscall

.section .data
buffer: .quad 0xdeadbeef

.section .rodata
file: .asciz "/hello_world.txt"
