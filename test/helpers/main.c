/* main.c - testsuite entry point */
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <unistd.h>

#include <macros.h>

#define EPILOGUE_SIZE	4096

static const char* _test_banner = "\n--------------------------------\n";

extern void (*__start_test_registry)(void);
extern void (*__stop_test_registry)(void);

static const char* results[] = {
	[_TEST_PASS] = "PASS", 
	[_TEST_FAIL] = "FAIL", 
	[_TEST_ERROR] = "ERROR"
};

static char* epilogue;

int main(void) {
	unsigned int fail = 0;
	unsigned int pass = 0;
	unsigned int error = 0;
	printf("%s", _test_banner);
	printf("Runing tests for %s\n\n", _test_name);
	pid_t runner_pid;
	int sts;
	enum _test_result_t res;
	const char* post_msg;
	epilogue = mmap(NULL, EPILOGUE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	for (struct _test_registered_t* test =
			(struct _test_registered_t*)&__start_test_registry;
			test < (struct _test_registered_t*)&__stop_test_registry; test++) {
		printf("Running %s ... ", test->name);
		fflush(stdout);
		*epilogue = 0;
		runner_pid = fork();
		if (runner_pid) {
			if (waitpid(runner_pid, &sts, 0) == -1 || !WIFEXITED(sts)) {
				res = _TEST_ERROR;
			}
			else if (WEXITSTATUS(sts) == EXIT_FAILURE) {
				res = _TEST_FAIL;
			}
			else {
				res = _TEST_PASS;
			}

			if (res == test->expect) {
				printf("\033[32m");
				if (res != _TEST_PASS) {
					post_msg = "*";
				}
				else {
					post_msg = "";
				}
				pass++;
			}
			else {
				printf("\033[31m");
				if (res == _TEST_ERROR) {
					error++;
				}
				else {
					fail++;
				}

				if (res == _TEST_PASS) {
					post_msg = "*";
				}
				else {
					post_msg = "";
				}
			}

			printf("%s%s %s\033[0m\n", results[res], post_msg, epilogue);
		}
		else {
			test->test();
			exit(0);
		}
	}

	if (pass) {
		printf("\n\033[32m%u PASSED\033[0m\n", pass);
	}

	if (fail) {
		printf("\033[31m%u FAILED\033[0m\n", fail);
	}

	if (error) {
		printf("\033[31m%u ERRORED\033[0m\n", error);
	}

	printf("%s", _test_banner);

	munmap(epilogue, EPILOGUE_SIZE);
	return (int)(fail + error);
}

void _test_report(const char* report) {
	strncpy(epilogue, report, EPILOGUE_SIZE);
}
