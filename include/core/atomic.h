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

typedef uint64_t MutexHandle;

void atomicinit(void);

MutexHandle kcreateMutex(void);

/* TODO: implement inter cpu locking */

void kacquireMutex(MutexHandle handle);

void kreleaseMutex(MutexHandle handle);

void ksti(void);

void kcli(void);

void setInterrupts(uint8_t set);

#endif /* CORE_ATOMIC_H */