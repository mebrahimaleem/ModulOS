/* pic.S - port io */
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

.globl io_wait
io_wait:
out %al, $0x80
ret

.globl outb
outb:
mov %di, %dx
mov %sil, %al
out %al, %dx
ret

.globl outw
outw:
mov %di, %dx
mov %si, %ax
out %ax, %dx
ret

.globl outd
outd:
mov %di, %dx
mov %esi, %eax
out %eax, %dx
ret

.globl inb
inb:
mov %di, %dx
xorq %rax, %rax
in %dx, %al
ret

.globl inw
inw:
mov %di, %dx
xorq %rax, %rax
in %dx, %ax
ret

.globl ind
ind:
mov %di, %dx
xorq %rax, %rax
in %dx, %eax
ret
