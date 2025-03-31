/* serial.S - serial port functions */
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

#ifndef CORE_PORTIO_S
#define CORE_PORTIO_S

/* void outb(uint16_t addr, uint8_t x); */
  .globl outb
outb:
  movw %di, %dx
  movw %si, %ax
  outb %al, %dx
  ret

/* void outw(uint16_t addr, uint16_t x); */
  .globl outw
outw:
  movw %di, %dx
  movw %si, %ax
  outw %ax, %dx

  ret
/* void outd(uint16_t addr, uint32_t x); */
  .globl outd
outd:
  movw %di, %dx
  movl %esi, %eax
  outl %eax, %dx
  ret

/* uint8_t inb(uint16_t addr); */
  .globl inb
inb:
  movw %di, %dx
  inb %dx, %al
  ret

/* uint16_t inw(uint16_t addr); */
  .globl inw
inw:
  movw %di, %dx
  inw %dx, %ax
  ret

/* uint32_t ind(uint16_t addr); */
  .globl ind
ind:
  movw %di, %dx
  inl %dx, %eax
  ret

/* void iowait(void); */
  .globl iowait
iowait:
  mov $0x80, %al
  outb %al, $0
  ret

#endif /* CORE_PORTIO_S */