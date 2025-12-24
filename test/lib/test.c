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

#include <kernel/lib/kmemset.h>
#include <kernel/lib/kmemcmp.h>
#include <kernel/lib/kmemcpy.h>
#include <kernel/lib/hash.h>

#define MEM_TEST_SIZE	256

TEST_NAME("libary test suite")

TEST("kmemset") {
	char* actual = malloc(MEM_TEST_SIZE);

	for (uint64_t i = 0; i < 256; i += (i % 7) + 1) {
		ASSERT_TRUE(actual == kmemset(actual, i, MEM_TEST_SIZE), "kernel memset return value changed");

	}

	free(actual);
}

TEST("kmemcmp") {
	ASSERT_TRUE(kmemcmp("", "", 0) == 0, "kernel memcmp fails zero size");
	ASSERT_TRUE(kmemcmp("", "", 1) == 0, "kernel memcmp fails empty string");
	ASSERT_TRUE(kmemcmp(" a", " b", 3) < 0, "kernel memcmp flips sign");
	ASSERT_TRUE(kmemcmp(" 4", " 2", 3) > 0, "kernel memcmp flips sign");
	ASSERT_TRUE(kmemcmp(" a", " b", 1) == 0, "kernel memcmp reads too many bytes");
	ASSERT_TRUE(kmemcmp("\0sneaky", "\0hidden", 8) > 0, "kernel memcmp stops on null");
	ASSERT_TRUE(kmemcmp("1234567", "12345", 5) == 0, "kernel memcmp reads too many bytes");
	ASSERT_TRUE(kmemcmp((void*)0xdeadbeef, (void*)0xdeadbeef, 0) == 0, "kernel memcmp fails invalid ptr");
}

TEST("kmemcpy") {
	char* original = malloc(MEM_TEST_SIZE);
	char* copy = malloc(MEM_TEST_SIZE);

	for (uint64_t i = 0; i < MEM_TEST_SIZE; i += (i % 7) + 1) {
		original[i] = (char)(i ^ (i * i));
	}

	kmemcpy(copy, original, MEM_TEST_SIZE);
	ASSERT_TRUE(!memcmp(original, copy, MEM_TEST_SIZE), "memory comparison fails");

	free(original);
	free(copy);
}

TEST("hash_byte_sum") {
	ASSERT_TRUE(hash_byte_sum((uint8_t*)0xdeadbeef, 0) == 0, "fails invalid ptr");
	ASSERT_TRUE(hash_byte_sum((uint8_t*)"\0\1\2\3\0\0\1\2", 8) == 9, "fails basic sum");
	ASSERT_TRUE(hash_byte_sum((uint8_t*)"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 8) == 248, "fails overflow");
}
