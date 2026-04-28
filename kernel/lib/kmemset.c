/* kmemset.c - library memset implementation */
/* Copyright (C) 2026  Ebrahim Aleem
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

#include <lib/kmemset.h>

#define PAT(s) (((uint64_t)v8) << (8*s))

void* memset(void* ptr, int32_t v, size_t c) __attribute__((weak));

void* memset(void* ptr, int32_t v, size_t c) {
	uint8_t v8 = (uint8_t)v;
	uint64_t v64 = PAT(0) | PAT(1) | PAT(2) | PAT(3) | PAT(4) | PAT(5) | PAT(6) | PAT(7);
	uint64_t* ptr64 = (uint64_t*)ptr;

	while (c >= sizeof(uint64_t)) {
		*ptr64 = v64;
		ptr64++;
		c -= sizeof(uint64_t);
	}

	uint8_t* ptr8 = (uint8_t*)ptr64;

	while (c >= sizeof(uint8_t)) {
		*ptr8 = v8;
		ptr8++;
		c -= sizeof(uint8_t);
	}

	return ptr;
}

void* kmemset(void* ptr, int32_t v, size_t c) __attribute__((alias("memset")));
