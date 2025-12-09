/* mem_utils.h - library memory utility interface */
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

#include <stdint.h>
#include <stddef.h>

extern void* memset(void* ptr, uint64_t v, size_t c);
extern void* memsetb(void* ptr, uint8_t v, size_t c);
extern void* memsetw(void* ptr, uint16_t v, size_t c);
extern void* memsetd(void* ptr, uint32_t v, size_t c);
extern void* memsetq(void* ptr, uint64_t v, size_t c);

extern void* memcpy(void* dest, uint64_t src, size_t c);
extern void* memcpyb(void* dest, uint64_t src, size_t c);
extern void* memcpyw(void* dest, uint64_t src, size_t c);
extern void* memcpyd(void* dest, uint64_t src, size_t c);
extern void* memcpyq(void* dest, uint64_t src, size_t c);
