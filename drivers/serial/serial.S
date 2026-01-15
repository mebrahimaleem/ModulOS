/* serial.S - serial driver driver */
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

#define COM1	0x3F8
#define COM2	0x2F8

#define	TRSM		0x00
#define RCSV		0x00
#define BAUD_LO	0x00
#define BAUD_HI	0x01
#define IER			0x01
#define FCR			0x02
#define LCR			0x03
#define MCR			0x04


#define LP_8N1	0x03
#define DIVISOR	0x03 // 115200 / 3 = 38400 baud
#define INT_DS	0x00
#define INT_EN	0x01
#define	DLAB		0x80
#define F_EN		0x01
#define F_CLTR	0x06 // clear trms & recv
#define F_SNGL	0x00
#define F_INT1B	0x00 //TODO: maybe increase thresh
#define	DTR			0x01
#define	RTS			0x02
#define OUT1		0x04
#define	OUT2		0x08


//TODO: test ports via loopback

.globl serial_init_com1
serial_init_com1:
mov $COM1, %rdi
jmp serial_init

.globl serial_init_com2
serial_init_com2:
mov $COM2, %rdi
jmp serial_init

serial_init:
movb $INT_DS, %al
lea IER(%rdi), %rdx
out %al, %dx

movb $DLAB, %al
lea LCR(%rdi), %rdx
out %al, %dx

movb $DIVISOR, %al
lea BAUD_LO(%rdi), %rdx
out %al, %dx

xorb %al, %al
lea BAUD_HI(%rdi), %rdx
out %al, %dx

movb $LP_8N1, %al
lea LCR(%rdi), %rdx
out %al, %dx

movb $(F_EN + F_CLTR + F_SNGL + F_INT1B), %al
lea FCR(%rdi), %rdx
out %al, %dx

movb $(DTR + RTS + OUT1 + OUT2), %al
lea MCR(%rdi), %rdx
out %al, %dx

mov INT_EN, %al
lea IER(%rdi), %rdx
out %al, %dx

ret

.globl serial_write_com1
serial_write_com1:
mov $(COM1 + TRSM), %dx
movb %dil, %al
out %al, %dx
ret

.globl serial_write_com2
serial_write_com2:
mov $(COM2 + TRSM), %dx
movb %dil, %al
out %al, %dx
ret

