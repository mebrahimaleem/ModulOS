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
#include <core/cpulowlevel.h>
#include <core/serial.h>
#include <core/memory.h>

#include <apic/lapic.h>

#define MAX_KSTATICMUTEX	0x20
#define LOCK							0x01
#define UNLOCK						0x00

extern uint8_t disableInterrupts;

static uint64_t kstaticmutexes[MAX_KSTATICMUTEX];

volatile uint64_t* kmutexes;

uint64_t knextMutex;

void atomicinit(void) {
	knextMutex = 0;
	kmutexes = kstaticmutexes;
	disableInterrupts = 1;
}

StaticMutexHandle kcreateStaticMutex() {
	kcli();
	kmutexes[knextMutex] = 0;
	knextMutex++;
	ksti();
	return knextMutex - 1;
}

void kacquireStaticMutex(StaticMutexHandle handle) {
	uint64_t exp;
	while (1) {
		kcli();
		exp = kxchg(&kmutexes[handle], LOCK);
		ksti();
		if (exp == UNLOCK) return;

		// fast inner busy wait to avoid cli too often
		while(kmutexes[handle] == LOCK);
	}
}

void kreleaseStaticMutex(StaticMutexHandle handle) {
	kcli();
	kxchg(&kmutexes[handle], UNLOCK);
	ksti();
}

void setInterrupts(uint8_t set) {
	disableInterrupts = set;
}

mutex_t kcreateMutex() {
	mutex_t ret = kmalloc(sizeof(_mutex_t));
	*ret = UNLOCK;
	return ret;
}

void kacquireMutex(mutex_t handle) {
	uint64_t exp;
	while (1) {
		kcli();
		exp = kxchg(handle, LOCK);
		ksti();
		if (exp == UNLOCK) break;

		// fast inner busy wait to avoid cli too often
		while(*(volatile mutex_t )handle == LOCK)
			pause();
	}
}

void kreleaseMutex(mutex_t handle) {
	kcli();
	kxchg(handle, UNLOCK);
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
	//TODO: implement bus locking
	while (1) {
		kcli();
		if (*handle >= count) {
			*handle -= count;
			ksti();
			return;
		}
		ksti();

		// fast inner busy wait to avoid cli too often
		while(*handle < count);
	}
}

void kreleaseSemaphore(semaphore_t handle, uint64_t count) {
	//TODO: implement bus locking
	kcli();
	*handle += count;
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
	//TODO: implement bus locking
	while (1) {
		kcli();
		if (*handle == 0) {
			*handle = 1;
			ksti();
			return;
		}
		ksti();

		// fast inner busy wait to avoid cli too often
		while(*handle == 1);
	}
}

void kreleaseSpinlock(spinlock_t handle) {
	//TODO: implement bus locking
	kcli();
	*handle = 0;
	ksti();
}

void kdeleteSpinlock(spinlock_t handle) {
	kfree(handle);
}

#endif /* CORE_ATOMIC_C */
