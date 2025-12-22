/* test.c - library test entry point */
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

#include <memory.h>
#include <stddef.h>
#include <stdint.h>

#include <macros.h>

#define memset k_memset
#include <kernel/lib/memset.h>

void* k_memset(void* ptr, uint64_t v, size_t len) __asm__("memset");

#define MEM_TEST_SIZE	256

TEST_NAME("libary test suite")

TEST("memset") {
	char* actual = malloc(MEM_TEST_SIZE);

	for (uint64_t i = 0; i < 256; i += (i % 7) + 1) {
		ASSERT_TRUE(actual == k_memset(actual, i, MEM_TEST_SIZE), "kernel memset return value changed");

	}
}
