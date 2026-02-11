/* ap.S - AP bootstrap routine */
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

#define	AP_LOCK			0x9000
#define AP_ID				0x9001
#define AP_STACK		0x9100
#define AP_GDT32		0x9002
#define AP_GDT64		0x9008
#define AP_PML4			0x9012
#define AP_STACKS		0x9016

#define CMOS_RAM_INDEX	0x70
#define CMOS_RAM_DATA		0x71
#define NMI_DISABLE			0x80

.section .data

.globl ap_bootstrap_start
ap_bootstrap_start:


.code16

// set known state
ljmp $0, $0x8005
xorw %ax, %ax
movw %ax, %ds
movw %ax, %ss
movw %ax, %es
movw %ax, %fs
movw %ax, %gs
movl $AP_STACK, %esp // identical stacks so concurrency is not a problem
pushw %ax
popf

// mask nmis
inb $CMOS_RAM_INDEX, %al
or $NMI_DISABLE, %al
outb %al, $CMOS_RAM_INDEX
out %al, $0x80
inb $CMOS_RAM_DATA, %al

// assign arbitration id
movb $1, %al
.wait_lock:
xchgb %al, AP_LOCK
test %al, %al
jnz .wait_lock
movb AP_ID, %bl
addb $1, %bl
movb %bl, AP_ID
xorb %al, %al
xchgb %al, AP_LOCK

// enable paging
movl %cr4, %eax
orl $0x30, %eax
movl %eax, %cr4

movl AP_PML4, %edi
movl %edi, %cr3

// enter compat
movl $0xC0000080, %ecx
rdmsr
orl $0x100, %eax
wrmsr

movl %cr0, %ecx
orl $0x80000001, %ecx
movl %ecx, %cr0

lgdt AP_GDT32

pushw $0
pushw $8
pushw $0
pushw $0x807d
lretl

// 0x2083

.code64

movw $0x10, %ax
movw %ax, %ds
movw %ax, %es
movw %ax, %fs
movw %ax, %gs
movw %ax, %ss

lgdt AP_GDT64

xorq %rdi, %rdi
movb %bl, %dil

movq AP_STACKS, %rax
movq (%rax,%rdi,8), %rsp

pushq $0
popfq

pushq $0x8
pushq $kapentry
lretq


.globl ap_bootstrap_end
ap_bootstrap_end:
