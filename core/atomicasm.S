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

/* uint64_t kxchg(uint64_t* ptr, uint64_t v); */
	.globl kxchg
kxchg:
	lock xchg (%rdi), %rsi
	mov %rsi, %rax
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


