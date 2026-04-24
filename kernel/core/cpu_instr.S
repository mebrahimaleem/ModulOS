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

.section .text

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

.globl cpu_hlt
cpu_hlt:
hlt
ret

.globl cpu_set_cr4
cpu_set_cr4:
movq %cr4, %rax
orq %rdi, %rax
movq %rax, %cr4
ret

.globl cpu_get_fsbase
cpu_get_fsbase:
rdfsbaseq %rax
ret

.globl cpu_set_fsbase
cpu_set_fsbase:
wrfsbaseq %rdi
ret

.globl cpu_save_fx
cpu_save_fx:
fxsave (%rdi)
ret

.globl cpu_restore_fx
cpu_restore_fx:
fxrstor (%rdi)
ret

.globl cpu_init_fx
cpu_init_fx:
movq %cr0, %rax
andw $0xFFFB, %ax
orw $2, %ax
movq %rax, %cr0

movq %cr4, %rax
orq $(3 << 9), %rax
movq %rax, %cr4
ret
