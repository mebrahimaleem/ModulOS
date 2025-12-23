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
#define memcmp k_memcmp
#include <kernel/lib/memset.h>
#include <kernel/lib/memcmp.h>
#include <kernel/lib/hash.h>

void* k_memset(void* ptr, uint64_t v, size_t len) __asm__("memset");
int64_t k_memcmp(const void* l, const void* r, size_t c) __asm__("memcmp");

#define MEM_TEST_SIZE	256

TEST_NAME("libary test suite")

TEST("memset") {
	char* actual = malloc(MEM_TEST_SIZE);

	for (uint64_t i = 0; i < 256; i += (i % 7) + 1) {
		ASSERT_TRUE(actual == k_memset(actual, i, MEM_TEST_SIZE), "kernel memset return value changed");

	}

	free(actual);
}

TEST("memcmp") {
	ASSERT_TRUE(k_memcmp("", "", 0) == 0, "kernel memcmp fails zero size");
	ASSERT_TRUE(k_memcmp("", "", 1) == 0, "kernel memcmp fails empty string");
	ASSERT_TRUE(k_memcmp(" a", " b", 3) < 0, "kernel memcmp flips sign");
	ASSERT_TRUE(k_memcmp(" 4", " 2", 3) > 0, "kernel memcmp flips sign");
	ASSERT_TRUE(k_memcmp(" a", " b", 1) == 0, "kernel memcmp reads too many bytes");
	ASSERT_TRUE(k_memcmp("\0sneaky", "\0hidden", 8) > 0, "kernel memcmp stops on null");
	ASSERT_TRUE(k_memcmp("1234567", "12345", 5) == 0, "kernel memcmp reads too many bytes");
	ASSERT_TRUE(k_memcmp((void*)0xdeadbeef, (void*)0xdeadbeef, 0) == 0, "kernel memcmp fails invalid ptr");
}

TEST("hash_byte_sum") {
	ASSERT_TRUE(hash_byte_sum((uint8_t*)0xdeadbeef, 0) == 0, "fails invalid ptr");
	ASSERT_TRUE(hash_byte_sum((uint8_t*)"\0\1\2\3\0\0\1\2", 8) == 9, "fails basic sum");
	ASSERT_TRUE(hash_byte_sum((uint8_t*)"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 8) == 248, "fails overflow");
}
