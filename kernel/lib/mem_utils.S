/* mem_utils.S - library memory utility routines */
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

.section .text

.globl memset
memset:
call memsetb
ret

.globl memcpy
memcpy:
call memcpyb
ret

//.globl memcmp
//memcmp:
// TODO: implement

//.globl memmove
//memmove:
// TODO: implement

.globl memsetb
memsetb:
pushq %rdi
movq %rsi, %rax
movq %rdx, %rcx
rep stosb
popq %rax
ret

.globl memsetw
memsetw:
pushq %rdi
movq %rsi, %rax
movq %rdx, %rcx
rep stosw
popq %rax
ret

.globl memsetd
memsetd:
pushq %rdi
movq %rsi, %rax
movq %rdx, %rcx
rep stosl
popq %rax
ret

.globl memsetq
memsetq:
pushq %rdi
movq %rsi, %rax
movq %rdx, %rcx
rep stosq
popq %rax
ret

.globl memcpyb
memcpyb:
pushq %rdi
movq %rdx, %rcx
rep movsb
popq %rax
ret

.globl memcpyw
memcpyw:
pushq %rdi
movq %rdx, %rcx
rep movsw
popq %rax
ret

.globl memcpyd
memcpyd:
pushq %rdi
movq %rdx, %rcx
rep movsl
popq %rax
ret

.globl memcpyq
memcpyq:
pushq %rdi
movq %rdx, %rcx
rep movsq
popq %rax
ret
