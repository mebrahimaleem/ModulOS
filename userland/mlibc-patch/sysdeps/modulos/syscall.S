/* syscall.S - userland system call transfer implementation */
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

.globl syscall_0
.globl syscall_1
.globl syscall_2
.globl syscall_3
.globl syscall_4
.globl syscall_5
.globl syscall_6

.section .text

syscall_6:
popq %r10
syscall_5:
syscall_4:
syscall_3:
syscall_2:
syscall_1:
syscall_0:
movq %rcx, %rax
syscall
ret
