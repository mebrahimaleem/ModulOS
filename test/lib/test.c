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

#include <stdlib.h>
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
#include <kernel/lib/hash_table.h>
#include <kernel/lib/array_list.h>

#define MEM_TEST_SIZE	256

TEST_NAME("libary test suite");

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
	ASSERT_TRUE(hash_byte_sum((const uint8_t*)"\0\1\2\3\0\0\1\2", 8) == 9, "fails basic sum");
	ASSERT_TRUE(hash_byte_sum((const uint8_t*)"\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 8) == 248, "fails overflow");
}

TEST("crc32_ansi") {
	ASSERT_TRUE(crc32_ansi("Hello, World!", 13) == 0xec4ac3d0, "fails expected checksum");
	ASSERT_FALSE(crc32_ansi("Hello, World!", 12) == 0xec4ac3d0, "fails unexpected checksum");
}

TEST("fnc64_1a") {
	ASSERT_TRUE(fnv64_1a("Hello, World!", 13) == 0x6ef05bd7cc857c54, "fails expected checksum");
	ASSERT_FALSE(fnv64_1a("Hello, World!", 12) == 0x6ef05bd7cc857c54, "fails expected checksum");
}

static void* hash_table_dup_fun(void* value) {
	return (void*)((uint64_t)value + 1);
}

TEST("hash_table") {
	struct hash_table_t* table1 = hash_table_alloc(10);
	struct hash_table_t* table2 = hash_table_alloc(10);
	void* tmp;

	ASSERT_TRUE(hash_table_insert(table1, 1, (void*)1) == (void*)1, "fails hash_table_insert");
	ASSERT_TRUE(hash_table_insert(table1, 2, (void*)2) == (void*)2, "fails hash_table_insert");
	ASSERT_TRUE(hash_table_insert(table1, 328642, (void*)3) == (void*)3, "fails hash_table_insert");
	ASSERT_TRUE(hash_table_insert(table1, ~834ULL, (void*)4) == (void*)4, "fails hash_table_insert");
	ASSERT_TRUE(hash_table_insert(table1, 1, (void*)5) == (void*)1, "fails hash_table_insert");
	ASSERT_TRUE(hash_table_insert(table1, 11, (void*)6) == (void*)6, "fails hash_table_insert");

	ASSERT_TRUE(hash_table_get(table1, 1, &tmp), "fails hash_table_get");
	ASSERT_TRUE(tmp == (void*)5, "fails hash_table_get");

	ASSERT_TRUE(hash_table_get(table1, 2, &tmp), "fails hash_table_get");
	ASSERT_TRUE(tmp == (void*)2, "fails hash_table_get");

	ASSERT_TRUE(hash_table_get(table1, 328642, &tmp), "fails hash_table_get");
	ASSERT_TRUE(tmp == (void*)3, "fails hash_table_get");

	ASSERT_TRUE(hash_table_get(table1, ~834ULL, &tmp), "fails hash_table_get");
	ASSERT_TRUE(tmp == (void*)4, "fails hash_table_get");

	ASSERT_TRUE(hash_table_get(table1, 11, &tmp), "fails hash_table_get");
	ASSERT_TRUE(tmp == (void*)6, "fails hash_table_get");

	ASSERT_FALSE(hash_table_get(table1, 3, &tmp), "fails hash_table_get");
	ASSERT_FALSE(hash_table_get(table1, 0, &tmp), "fails hash_table_get");
	ASSERT_FALSE(hash_table_get(table2, 0, &tmp), "fails hash_table_get");

	ASSERT_TRUE(hash_table_remove(table1, 1, &tmp), "fails hash_table_remove");
	ASSERT_TRUE(tmp == (void*)5, "fails hash_table_remove");
	tmp = (void*)0;
	ASSERT_FALSE(hash_table_remove(table1, 1, &tmp), "fails hash_table_remove");
	ASSERT_TRUE(tmp == (void*)0, "fails hash_table_remove");

	ASSERT_TRUE(hash_table_get(table1, 11, &tmp), "fails hash_table_get after remove");
	ASSERT_FALSE(hash_table_remove(table2, 1, &tmp), "fails hash_table_remove");

	hash_table_copy(table1, table2, hash_table_dup_fun);
	hash_table_clear(table1, 0);

	ASSERT_TRUE(hash_table_count(table1) == 0, "fails hash_table_count");
	ASSERT_TRUE(hash_table_count(table2) == 4, "fails hash_table_count");

	ASSERT_FALSE(hash_table_get(table1, 11, &tmp), "fails hash_table_get after clear");
	ASSERT_TRUE(hash_table_get(table2, 11, &tmp), "fails hash_table_get after copy");
	ASSERT_TRUE(tmp == (void*)7, "fails hash_table_get after copy");

	hash_table_insert(table2, 1, (void*)8);
	hash_table_insert(table2, 101, (void*)9);
	hash_table_resize(table2, 100);

	ASSERT_TRUE(hash_table_get(table2, 1, &tmp), "fails hash_table_get after resize");
	ASSERT_TRUE(tmp == (void*)8, "fails hash_table_get after resize");

	ASSERT_TRUE(hash_table_get(table2, 2, &tmp), "fails hash_table_get after resize");
	ASSERT_TRUE(tmp == (void*)3, "fails hash_table_get after resize");

	ASSERT_TRUE(hash_table_get(table2, 328642, &tmp), "fails hash_table_get after resize");
	ASSERT_TRUE(tmp == (void*)4, "fails hash_table_get after resize");

	ASSERT_TRUE(hash_table_get(table2, ~834ULL, &tmp), "fails hash_table_get after resize");
	ASSERT_TRUE(tmp == (void*)5, "fails hash_table_get after resize");

	ASSERT_TRUE(hash_table_get(table2, 11, &tmp), "fails hash_table_get after resize");
	ASSERT_TRUE(tmp == (void*)7, "fails hash_table_get after resize");

	ASSERT_TRUE(hash_table_get(table2, 101, &tmp), "fails hash_table_get after resize");
	ASSERT_TRUE(tmp == (void*)9, "fails hash_table_get after resize");

	hash_table_free(table1, 0);
	hash_table_free(table2, 0);
}

static void* array_list_dup_fun(void* value) {
	return (void*)((uint64_t)value + 1);
}

TEST("array_list") {
	struct array_list_t* list1 = array_list_alloc(1, 1, (void*)2);
	struct array_list_t* list2;

	ASSERT_TRUE(array_list_count(list1) == 0, "fails array_list_count");

	ASSERT_TRUE(array_list_get(list1, 0) == (void*)2, "fails array_list_get");
	ASSERT_TRUE(array_list_get(list1, 1) == (void*)2, "fails array_list_get");

	ASSERT_TRUE(array_list_push(list1, (void*)101) == 0, "fails array_list_push");
	ASSERT_TRUE(array_list_push(list1, (void*)201) == 1, "fails array_list_push");
	ASSERT_TRUE(array_list_push(list1, (void*)301) == 2, "fails array_list_push");

	ASSERT_TRUE(array_list_get(list1, 0) == (void*)101, "fails array_list_get");
	ASSERT_TRUE(array_list_get(list1, 1) == (void*)201, "fails array_list_get");
	ASSERT_TRUE(array_list_get(list1, 100) == (void*)2, "fails array_list_get");

	ASSERT_TRUE(array_list_remove(list1, 1) == (void*)201, "fails array_list_remove");
	ASSERT_TRUE(array_list_remove(list1, 3) == (void*)2, "fails array_list_remove");
	ASSERT_TRUE(array_list_remove(list1, 100) == (void*)2, "fails array_list_remove");

	ASSERT_TRUE(array_list_push(list1, (void*)401) == 1, "fails array_list_push");

	list2 = array_list_dup(list1, array_list_dup_fun);
	array_list_clear(list1, 0);

	ASSERT_TRUE(array_list_count(list1) == 0, "fails array_list_count after clear");
	ASSERT_TRUE(array_list_count(list2) == 3, "fails array_list_count after copy");

	ASSERT_TRUE(array_list_push(list1, (void*)501) == 0, "fails array_list_push after clear");

	ASSERT_TRUE(array_list_get(list1, 0) == (void*)501, "fails array_list_get after clear");
	ASSERT_TRUE(array_list_get(list1, 1) == (void*)2, "fails array_list_get after clear");
	ASSERT_TRUE(array_list_get(list1, 100) == (void*)2, "fails array_list_get after clear");

	ASSERT_TRUE(array_list_get(list2, 0) == (void*)101, "fails array_list_get after copy");
	ASSERT_TRUE(array_list_get(list2, 1) == (void*)401, "fails array_list_get after copy");
	ASSERT_TRUE(array_list_get(list2, 100) == (void*)2, "fails array_list_get after copy");

	array_list_free(list1, 0);
	array_list_free(list2, 0);
}
