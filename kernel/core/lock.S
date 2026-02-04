/* lock.S - spinlock routines */
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

.globl lock_acquire
lock_acquire:
movq $1, %rax
.loop:
pause
xchgb %al, (%rdi)
testq %rax, %rax
jnz .loop
ret

.globl lock_release
lock_release:
xorq %rax, %rax
xchgb %al, (%rdi)
ret
