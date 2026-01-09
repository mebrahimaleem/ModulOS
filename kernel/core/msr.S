/* msr.S - Model specific register routines */
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

.globl msr_write
msr_write:
movl %esi, %eax
movq %rsi, %rdx
shrq $32, %rdx
movl %edi, %ecx
wrmsr
ret

.globl msr_read
msr_read:
movl %edi, %ecx
rdmsr
shlq $32, %rdx
orq %rdx, %rax
ret
