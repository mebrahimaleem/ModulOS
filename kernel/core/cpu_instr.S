/* cpu_instr.S - cpu instruction routines */
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

.globl cpu_lidt
cpu_lidt:
lidt (%rdi)
ret

.globl cpu_ltr_28
cpu_ltr_28:
mov $0x28, %rax
ltr %ax
ret

.globl cpu_cli
cpu_cli:
cli
ret

.globl cpu_sti
cpu_sti:
sti
ret

.globl cpu_pause
cpu_pause:
pause
ret

.globl cpu_invlpg
cpu_invlpg:
invlpg (%rdi)
ret

.globl cpu_wait_loop
cpu_wait_loop:
hlt
jmp cpu_wait_loop

.globl cpu_halt_loop
cpu_halt_loop:
cli
hlt
jmp cpu_halt_loop

.globl cpu_trap
cpu_trap:
ret

.globl cpu_read_cr2
cpu_read_cr2:
movq %cr2, %rax
ret

.globl cpu_wbinvd
cpu_wbinvd:
wbinvd
ret

.globl cpu_get_cr3
cpu_get_cr3:
movq %cr3, %rax
ret

.globl cpu_set_cr3
cpu_set_cr3:
testq %rdi, %rdi
jz skip_cr3
movq %rdi, %cr3
skip_cr3:
ret
