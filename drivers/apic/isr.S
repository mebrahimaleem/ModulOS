/* isr.S - APIC ISRs */
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

.globl apic_isr_timer_init
apic_isr_timer_init:
// TODO
iretq

.globl apic_isr_error
apic_isr_error:
movq %rsp, %rax
andq $0xF, %rax
jz .aligned
subq %rax, %rsp
.aligned:
call apic_error_dispatch
iretq
