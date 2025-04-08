/* atomicasm.S - cpu interrupt controlling routines */
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

#ifndef CORE_ATOMICASM_S
#define CORE_ATOMICASM_S

  .globl disableInterrupts
disableInterrupts:
.byte 0

buslock:
	.byte 0

/* void kunlockBus(void); */
	.globl kunlockBus
kunlockBus:
	movb $0,  %al
	lock xchg buslock, %al
	ret

/* void klockBus(void); */
	.globl klockBus
klockBus:
	movb $1, %al
klockBus.buslock:
	lock xchgb buslock, %al
	testb %al, %al
	jnz klockBus.buslock
	ret


/* void ksti(void); */
  .globl ksti
ksti:
  movb (disableInterrupts), %al
  testb %al, %al
  jnz ksti.nosti
  sti
ksti.nosti:
  ret

/* void kcli(void); */
  .globl kcli
kcli:
  cli
  ret

#endif /* CORE_ATOMICASM_S*/


