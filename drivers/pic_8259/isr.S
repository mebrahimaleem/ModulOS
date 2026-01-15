/* isr.S - PIC spurious ISR */
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

#define PIC1_CMD	0x20
#define PIC_EOI		0x20

.globl pic_spurious_master
pic_spurious_master:
iretq

.globl pic_spurious_slave
pic_spurious_slave:
movb $PIC_EOI, %al
outb %al, $PIC1_CMD
iretq
