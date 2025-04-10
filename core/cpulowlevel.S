/* cpulowlevel.S - cpu low level assembly routines */
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


#ifndef CORE_CPULOWLEVEL_S
#define CORE_CPULOWLEVEL_S

	.code64
	.text

/* void wbinvd(void); */
	.globl wbinvd
wbinvd:
	wbinvd
	ret

/* void cpuGetMSR(uint32_t msr, uint32_t* lo, uint32_t* hi); */
	.globl cpuGetMSR
cpuGetMSR:
	movl %edi, %ecx
	movq %rdx, %r10
	rdmsr
	movl %eax, (%rsi)
	movl %edx, (%r10)
	ret

/* void cpuSetMSR(uint32_t msr, uint32_t lo, uint32_t hi); */
	.globl cpuSetMSR
cpuSetMSR:
	movl %edi, %ecx
	movl %esi, %eax
	/* edx already set */
	wrmsr
	ret

/* void cpuid(uint32_t ieax, uint32_t iecx, uint32_t* oeax, uint32_t* oebx, uint32_t* oecx, uint32_t* oedx); */
	.globl cpuid
cpuid:
	pushq %rbx
	movl %edi, %eax
	movl %esi, %ecx
	movq %rdx, %r10
	movq %rcx, %r11
	cpuid
	movl %eax, (%r10)
	movl %ebx, (%r11)
	movl %ecx, (%r8)
	movl %edx, (%r9)
	popq %rbx
	ret

/* void ltr(void); */
	.globl ltr
ltr:
	movl $0x40, %eax
	ltr %ax
	ret

/* void pause(void); */
	.globl pause
pause:
	pause
	ret

#endif /* CORE_CPULOWLEVEL_S */

