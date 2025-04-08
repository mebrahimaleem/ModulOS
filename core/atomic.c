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
#include <core/memory.h>

extern uint8_t disableInterrupts;

#define MAX_KSTATICMUTEX 0x1000

static uint8_t kstaticmutexes[MAX_KSTATICMUTEX / sizeof(uint8_t)];

uint8_t* kmutexes;

uint64_t knextMutex;
uint64_t knumMutex;

void atomicinit(void) {
	knextMutex = 0;
	knumMutex = MAX_KSTATICMUTEX;
	kmutexes = kstaticmutexes;
	disableInterrupts = 1;
}

StaticMutexHandle kcreateStaticMutex() {
	kcli();
	klockBus();
	kmutexes[knextMutex / sizeof(uint8_t)] &= (uint8_t)~(1 << (knextMutex % sizeof(uint8_t))); // clear mutex bit
	knextMutex++;
	kunlockBus();
	ksti();
	return knextMutex - 1;
}

void kacquireStaticMutex(StaticMutexHandle handle) {
	const uint64_t i = handle / sizeof(uint8_t);
	const uint8_t mask = 1 << (handle % sizeof(uint8_t));
	
	while (1) {
		kcli();
		klockBus();
		if ((kmutexes[i] & mask) == 0) {
			kmutexes[i] |= mask;
			kunlockBus();
			ksti();
			return;
		}
		kunlockBus();
		ksti();

		// fast inner busy wait to avoid cli too often
		while((kmutexes[i] & mask) == mask);
	}
}

void kreleaseStaticMutex(StaticMutexHandle handle) {
	kcli();
	klockBus();
	kmutexes[handle / sizeof(uint8_t)] &= (uint8_t)~(1 << (handle % sizeof(uint8_t))); // clear mutex bit
	kunlockBus();
	ksti();
}

void setInterrupts(uint8_t set) {
	disableInterrupts = set;
}

mutex_t kcreateMutex() {
	mutex_t ret = kmalloc(sizeof(_mutex_t));
	*ret = 0;
	return ret;
}

void kacquireMutex(mutex_t handle) {
	while (1) {
		kcli();
		klockBus();
		if (*handle == 0) {
			*handle = 1;
			kunlockBus();
			ksti();
			return;
		}
		kunlockBus();
		ksti();

		// fast inner busy wait to avoid cli too often
		while(*handle == 1);
	}
}

void kreleaseMutex(mutex_t handle) {
	kcli();
	klockBus();
	*handle = 0;
	kunlockBus();
	ksti();
}

void kdeleteMutex(mutex_t handle) {
	kfree(handle);
}

semaphore_t kcreateSemaphore(uint64_t initial, uint64_t max) {
	semaphore_t ret = kmalloc(sizeof(_semaphore_t));
	*ret = max - initial;
	return ret;
}

void kacquireSemaphore(semaphore_t handle, uint64_t count) {
	while (1) {
		kcli();
		klockBus();
		if (*handle >= count) {
			*handle -= count;
			kunlockBus();
			ksti();
			return;
		}
		kunlockBus();
		ksti();

		// fast inner busy wait to avoid cli too often
		while(*handle < count);
	}
}

void kreleaseSemaphore(semaphore_t handle, uint64_t count) {
	kcli();
	klockBus();
	*handle += count;
	kunlockBus();
	ksti();
}

void kdeleteSemaphore(semaphore_t handle) {
	kfree(handle);
}

spinlock_t kcreateSpinlock() {
	spinlock_t ret = kmalloc(sizeof(_spinlock_t));
	*ret = 0;
	return ret;
}

void kacquireSpinlock(spinlock_t handle) {
	while (1) {
		kcli();
		klockBus();
		if (*handle == 0) {
			*handle = 1;
			kunlockBus();
			ksti();
			return;
		}
		kunlockBus();
		ksti();

		// fast inner busy wait to avoid cli too often
		while(*handle == 1);
	}
}

void kreleaseSpinlock(spinlock_t handle) {
	kcli();
	klockBus();
	*handle = 0;
	kunlockBus();
	ksti();
}

void kdeleteSpinlock(spinlock_t handle) {
	kfree(handle);
}

#endif /* CORE_ATOMIC_C */
