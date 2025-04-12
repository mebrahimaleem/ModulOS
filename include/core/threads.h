/* threads.h - multithreading routines */
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

#ifndef CORE_THREADS_H
#define CORE_THREADS_H

uint64_t thread_generatePID(void);

/* 
 * creates a kernel thread from an address (usually of a routine or function with no arguments)
 * sets up a stack and sets all non system registers to zero
*/
struct PCB* thread_kernelFromAddress(uint64_t addr);

#endif /* CORE_THREADS_H */

