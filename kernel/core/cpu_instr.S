/* cpu_instr.S - cpu instruction routines */
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

.globl cpu_lidt
cpu_lidt:
lidt (%rdi)
ret

.globl cpu_ltr_28
cpu_ltr_28:
mov $0x28, %rax
ltr %rax
ret

.globl cpu_cli
cpu_cli:
cli
ret

.globl cpu_sti
cpu_sti:
sti
ret
