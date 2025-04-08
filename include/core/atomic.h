/* atomic.h - atomic functions */
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

#ifndef CORE_ATOMIC_H
#define CORE_ATOMIC_H

#include <stdint.h>

typedef uint64_t StaticMutexHandle;

typedef uint64_t _spinlock_t;
typedef uint64_t _semaphore_t;
typedef uint64_t _mutex_t;

typedef _spinlock_t* spinlock_t;
typedef _semaphore_t* semaphore_t;
typedef _mutex_t* mutex_t;

void atomicinit(void);

StaticMutexHandle kcreateStaticMutex(void);

/* TODO: implement inter cpu locking */

void kacquireStaticMutex(StaticMutexHandle handle);

void kreleaseStaticMutex(StaticMutexHandle handle);

void ksti(void);

void kcli(void);

void kunlockBus(void);
void klockBus(void);

void setInterrupts(uint8_t set);

mutex_t kcreateMutex(void);
void kacquireMutex(mutex_t handle);
void kreleaseMutex(mutex_t handle);
void kdeleteMutex(mutex_t handle);

semaphore_t kcreateSemaphore(uint64_t initial, uint64_t max);
void kacquireSemaphore(semaphore_t handle, uint64_t count);
void kreleaseSemaphore(semaphore_t handle, uint64_t count);
void kdeleteSemaphore(semaphore_t handle);

spinlock_t kcreateSpinlock(void);
void kacquireSpinlock(spinlock_t handle);
void kreleaseSpinlock(spinlock_t handle);
void kdeleteSpinlock(spinlock_t handle);

#endif /* CORE_ATOMIC_H */
