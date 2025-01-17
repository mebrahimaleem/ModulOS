/* boot.S - bootstrap the kernel */
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

#define ASM_FILE

#ifndef MULTIBOOT2_BOOT_S
#define MULTIBOOT2_BOOT_S

#define MULTIBOOT2_MAGIC	0xE85250D6
#define ARCH_I386_PM			0

#define MB2TAG_TYPE_MEMMAP	6

#define STACK_SIZE				0x4000

	.code32
	.section .text.multiboot

	.align	8
multiboot_header:
	/* magic */
	.long		MULTIBOOT2_MAGIC

	/* i386 arch */
	.long		ARCH_I386_PM

	/* length */
	.long		multiboot_header_end - multiboot_header

	/* checksum */
	.long		-(MULTIBOOT2_MAGIC + ARCH_I386_PM + (multiboot_header_end - multiboot_header))

	/* information request (memmap) tag */
multiboot_header.memmap_start:
	.short	1
	.short	0
	.long		multiboot_header.memmap_end - multiboot_header.memmap_start
	.long		MB2TAG_TYPE_MEMMAP
multiboot_header.memmap_end:

	/* terminate tags */
	.short	0
	.long		8
multiboot_header_end:

	.section .text.boot
	.globl _start
_start:
	/* init stack */
	movl $(0x8000 + STACK_SIZE), %esp /* place stack right after gdt */
	movl %esp, %ebp

	/* eflags */
	pushl $0
	popfl

	jmp	pm2long

#endif /* MULTIBOOT2_BOOT_S */
