/* atomic.c - atomic functions */
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

#ifndef CORE_ATOMIC_C
#define CORE_ATOMIC_C

#include <core/atomic.h>

extern uint8_t disableInterrupts;

// max mutex needs to be constant because a dynamic amount would need an atomic heap which needs a mutex -> self dependency
#define MAX_KMUTEX 0x1000

static uint8_t kmutexes[MAX_KMUTEX / sizeof(uint8_t)];

uint64_t knextMutex;

void atomicinit(void) {
	knextMutex = 0;
	disableInterrupts = 1;
}

MutexHandle kcreateMutex() {
	kcli();
	kmutexes[knextMutex / sizeof(uint8_t)] &= (uint8_t)~(1 << (knextMutex % sizeof(uint8_t))); // clear mutex bit
	knextMutex++;
	ksti();
	return knextMutex - 1;
}

void kacquireMutex(MutexHandle handle) {
	const uint64_t i = handle / sizeof(uint8_t);
	const uint8_t mask = 1 << (handle % sizeof(uint8_t));
	
	while (1) {
		kcli();
		if ((kmutexes[i] & mask) == 0) {
			kmutexes[i] |= mask;
			ksti();
			return;
		}
		ksti();

		// fast inner busy wait to avoid cli too often
		while((kmutexes[i] & mask) == mask);
	}
}

void kreleaseMutex(MutexHandle handle) {
	kcli();
	kmutexes[handle / sizeof(uint8_t)] &= (uint8_t)~(1 << (handle % sizeof(uint8_t))); // clear mutex bit
	ksti();
}

void setInterrupts(uint8_t set) {
	disableInterrupts = set;
}

#endif /* CORE_ATOMIC_C */
