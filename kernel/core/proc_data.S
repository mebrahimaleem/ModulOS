/* proc_data.S - per procesor data routines */
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

.section .text

.globl proc_data_get
proc_data_get:
movq %gs:0, %rax
ret

.globl proc_data_set_id
proc_data_set_id:
movq proc_data_ptr, %rax
movq (%rax,%rdi,8), %rax
movl $0xC0000101, %ecx
movq %rax, %rdx
shrq $32,%rdx
wrmsr
ret

.section .data

.globl proc_arb_id
proc_arb_id:
.byte 0

.globl proc_data_ptr
proc_data_ptr:
.quad 0
