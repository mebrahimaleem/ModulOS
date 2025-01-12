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

#ifndef MULTIBOOT_BOOT_S
#define MULTIBOOT_BOOT_S

#define MULTIBOOT2_MAGIC	0xE85250D6
#define ARCH_I386_PM			0

#define FRAMEBUFFER_TAG		5
#define TAG_NOOPT					0
#define TAG_END						0

#define STACK_SIZE				0x4000

	.code32
	.section .multiboot

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

	/* terminate tags */
	.short	TAG_END
	.long		8
multiboot_header_end:

	.text
	.globl _start
_start:
	/* init stack */
	movl		$(stack + STACK_SIZE), %esp
	movl		%esp, %ebp

	/* eflags */
	pushl		$0
	popf

	jmp			pm2long

	/* stack */
	.comm		stack, STACK_SIZE

#endif /* MULTIBOOT_BOOT_S */
