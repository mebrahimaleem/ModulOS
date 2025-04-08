/* calibrate.S - precision calibration routines */
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

#ifndef APIC_CALIBRATE_S
#define APIC_CALIBRATE_S

#define BUSY	$0x0
#define IDLE	$0x1

#define INITIAL	$0x0
#define SECOND	$0x1

#define LAPIC_EOI_OFF		0x0B0
#define LAPIC_ICD_OFF		0x380
#define LAPIC_CCD_OFF		0x390

#define LAPIC_EOI				$0x00000000
#define LAPIC_TIMR			$0xFFFFFFFF
#define LAPIC_DISABLE		$0x00000000

#define PIT_DATA		$0x40
#define RELOAD_LO		$0x9C
#define RELOAD_HI		$0x2E

/* void calibrate_lapic_timer(uint32_t gsi, uint64_t apicbase); */
	.globl calibrate_lapic_timer
calibrate_lapic_timer:
	pushq %rbx
	// set status to initial
	movb BUSY, calibrate_done
	movb INITIAL, calibrate_sts

	/* reset PIT */
	movb RELOAD_LO, %al
  outb %al, PIT_DATA
	movb RELOAD_HI, %al
  outb %al, PIT_DATA

	pushq %rdi
	call apic_unmaskIrq
	popq %rdi

	// wait for status to become 0 before continuing
calibrate_lapic_timer.wait:
	movb calibrate_done, %cl
	testb %cl, %cl
	jz calibrate_lapic_timer.wait
	call apic_maskIrq

	movl %ebx, %eax
	popq %rbx
	ret

calibrate_done:
	.byte 0
calibrate_sts:
	.byte 0

	.globl lowlatency_isr
lowlatency_isr:
	movb calibrate_sts, %cl
	testb %cl, %cl

	jz lowlatency_isr.initial

	/* record apic timer value */
	movl LAPIC_CCD_OFF(%rsi), %ebx

	/* disable apic timer */
	movl LAPIC_DISABLE, LAPIC_ICD_OFF(%rsi)

	movb IDLE, calibrate_done

	/* send EOI */
	movl LAPIC_EOI, LAPIC_EOI_OFF(%rsi)
	iretq

lowlatency_isr.initial:
	movb SECOND, calibrate_sts

	/* start apic timer in one shot with really large countdown */
	movl LAPIC_TIMR, LAPIC_ICD_OFF(%rsi)

	/* send EOI */
	movl LAPIC_EOI, LAPIC_EOI_OFF(%rsi)
	iretq


#endif /* APIC_CALIBRATE_S*/


