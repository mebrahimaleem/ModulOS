/* test.c - testsuite test entry point */
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

#include <macros.h>

static void will_throw(void) {
	*(int*)(0xdeadbeef) = 0;
}

static int wont_throw(int a, int b, int c) {
	return a + b + c;
}

static void nested_exception(void) {
	ASSERT_NO_EXCEPTION(will_throw, "nested exception caught");
}

TEST_NAME("testsuite validation")

TEST("Implicit pass") {}

TEST_EXP_PASS("Expected pass") {
	TEST_PASS();
}

TEST_EXP_FAIL("Expected fail") {
	TEST_FAIL();
}

TEST_EXP_ERROR("Expected error") {
	will_throw();
}

TEST("Assert true #1") {
	ASSERT_TRUE(1 == 1, "");
}

TEST_EXP_FAIL("Assert true #2") {
	ASSERT_TRUE(1 == 0, "");
}

TEST("Assert false #1") {
	ASSERT_FALSE(34 == 12, "");
}

TEST_EXP_FAIL("Assert false #2") {
	ASSERT_FALSE(543, "");
}

TEST("Assert exception") {
	ASSERT_EXCEPTION(will_throw, "");
}

TEST("Assert no exception") {
	ASSERT_NO_EXCEPTION(wont_throw, "", 1, 2, 3);
}

TEST("Nested exception") {
	ASSERT_NO_EXCEPTION(nested_exception, "");
}
