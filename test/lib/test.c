/* test.c - library test entry point */
/* Copyright (C) 2025-2026  Ebrahim Aleem
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
#include <string.h>

#include <macros.h>

#include <kernel/lib/kmemset.h>
#include <kernel/lib/kmemcmp.h>
#include <kernel/lib/kmemcpy.h>
#include <kernel/lib/kstrlen.h>
#include <kernel/lib/kstrcpy.h>
#include <kernel/lib/kstrcmp.h>
#include <kernel/lib/hash.h>

#define MEM_TEST_SIZE	256

TEST_NAME("libary test suite")

TEST("kmemset") {
	char* actual = malloc(MEM_TEST_SIZE);

	for (int32_t i = 0; i < 256; i += (i % 7) + 1) {
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

TEST("kstrlen") {
	ASSERT_TRUE(kstrlen("") == 0, "fails empty string");
	ASSERT_TRUE(kstrlen("1") == 1, "fails 1 string");
	ASSERT_TRUE(kstrlen("12") == 2, "fails 12 string");
	ASSERT_TRUE(kstrlen("\t\n\r\0") == 3, "fails control character string");
}

TEST("kstrcpy") {
	char* original = malloc(MEM_TEST_SIZE);
	char* copy = malloc(MEM_TEST_SIZE);

	for (uint64_t i = 0; i < MEM_TEST_SIZE - 1; i += (i % 7) + 1) {
		original[i] = (char)(i ^ (i * i)) | 1;
	}

	kstrcpy(copy, original);
	ASSERT_TRUE(!strcmp(original, copy), "string comparison fails");

	free(original);
	free(copy);
}

TEST("kstrcpy_no_null") {
	char* original = malloc(MEM_TEST_SIZE);
	char* copy = malloc(MEM_TEST_SIZE);

	for (uint64_t i = 0; i < MEM_TEST_SIZE - 1; i += (i % 7) + 1) {
		original[i] = (char)(i ^ (i * i)) | 1;
	}
	copy[MEM_TEST_SIZE - 1] = 0xAA;

	kstrcpy_no_null(copy, original);
	ASSERT_TRUE(!memcmp(original, copy, strlen(original)), "string comparison fails");
	ASSERT_TRUE(copy[MEM_TEST_SIZE - 1] == (char)0xAA, "overwritten ending byte");

	free(original);
	free(copy);
}

TEST("kstrcmp") {
	ASSERT_TRUE(kstrcmp("", "") == 0, "kernel memcmp fails zero size");
	ASSERT_TRUE(kstrcmp(" a", " b") < 0, "kernel strcmp flips sign");
	ASSERT_TRUE(kstrcmp(" 4", " 2") > 0, "kernel strcmp flips sign");
	ASSERT_TRUE(kstrcmp(" a\0b", " a\0c") == 0, "kernel strcmp reads too many bytes");
}

TEST("hash_byte_sum") {
	ASSERT_TRUE(hash_byte_sum((uint8_t*)0xdeadbeef, 0) == 0, "fails invalid ptr");
	ASSERT_TRUE(hash_byte_sum((uint8_t*)"\0\1\2\3\0\0\1\2", 8) == 9, "fails basic sum");
	ASSERT_TRUE(hash_byte_sum((uint8_t*)"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 8) == 248, "fails overflow");
}

TEST("crc32_ansi") {
	ASSERT_TRUE(crc32_ansi("Hello, World!", 13) == 0xec4ac3d0, "fails expected checksum");
	ASSERT_FALSE(crc32_ansi("Hello, World!", 12) == 0xec4ac3d0, "fails unexpected checksum");
}
