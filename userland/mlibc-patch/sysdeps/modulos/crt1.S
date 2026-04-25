/* crt1.S - C runtime init */
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

.section .text
.global _start
_start:
	fninit
	emms
	ldmxcsr _mxcsr_reset

	movq %rsp, %rdi
	movq $main, %rsi
	call __mlibc_entry
.section .note.GNU-stack,"",%progbits

.section .data
_mxcsr_reset: .quad 0x1F80

.section .bss
.globl __dso_handle
__dso_handle: .quad 0
