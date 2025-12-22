/* macros.h - test suite macros interface */
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

#ifndef TEST_HELPERS_MACROS_H
#define TEST_HELPERS_MACROS_H

#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

enum _test_result_t {
	_TEST_PASS,
	_TEST_FAIL,
	_TEST_ERROR
};

struct _test_registered_t {
	const char* name;
	void  (*test)(void);
	enum _test_result_t expect;
};

extern const char* _test_name;

extern void _test_report(const char* report);

#define TEST_NAME(name) const char* _test_name = name;

#define TEST_EXP_PASS(nm) _TEST_EXPAND(__COUNTER__, nm, _TEST_PASS)
#define TEST_EXP_FAIL(nm) _TEST_EXPAND(__COUNTER__, nm, _TEST_FAIL)
#define TEST_EXP_ERROR(nm) _TEST_EXPAND(__COUNTER__, nm, _TEST_ERROR)

#define TEST(nm) TEST_EXP_PASS(nm)

#define _TEST_EXPAND(id, nm, exp) _TEST(id, nm, exp)

#define _TEST(id, nm, exp) \
	static void _test_##id(void); \
	static struct _test_registered_t _reg_##id __attribute__((used, section("test_registry"))) = { \
		.name = nm, \
		.test = _test_##id, \
		.expect = exp \
	}; \
	static void _test_##id(void)

#define TEST_PASS() exit(EXIT_SUCCESS)
#define TEST_FAIL(code) exit(EXIT_FAILURE)

#define ASSERT_TRUE(condition, report) \
	if (!(condition)) { \
		_test_report(report); \
		TEST_FAIL(); \
	}

#define ASSERT_FALSE(condition, report) \
	if ((condition)) { \
		_test_report(report); \
		TEST_FAIL(); \
	}

#define ASSERT_EXCEPTION(callback, report, ...) \
	do { \
		int sts; \
		pid_t pid = fork(); \
		if (pid) { \
				if (waitpid(pid, &sts, 0) != -1 && WIFEXITED(sts)) { \
					_test_report(report); \
					TEST_FAIL(); \
				} \
		} \
		else { \
			callback(__VA_OPT__(__VA_ARGS__)); \
			exit(0); \
		} \
	} while (0);

#define ASSERT_NO_EXCEPTION(callback, report, ...) \
	do { \
		int sts; \
		pid_t pid = fork(); \
		if (pid) { \
				if (waitpid(pid, &sts, 0) == -1 || !WIFEXITED(sts)) { \
					_test_report(report); \
					TEST_FAIL(); \
				} \
		} \
		else { \
			callback(__VA_ARGS__); \
			exit(0); \
		} \
	} while (0);

#endif /* TEST_HELPERS_MACROS_H */
