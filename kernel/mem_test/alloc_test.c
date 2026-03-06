/* alloc_test.c - heap allocator runtime testing implementation */
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

#include <mem_test/alloc_test.h>

#include <core/alloc.h>
#include <core/logging.h>
#include <core/panic.h>

#define NUM_BATCH	16

#define INTEGRITY_NUM	(((i * 131) ^ (j * 17) ^ alloc_size) & 0xFF)

void alloc_test_assert(void) {
	uint8_t* batch0[NUM_BATCH];
	uint8_t* batch1[NUM_BATCH];
	uint8_t* batch2[NUM_BATCH];

	uint64_t alloc_size, i, j;

	/* populate batch0 */
	alloc_size = 1;
	for (i = 0; i < NUM_BATCH; i++) {
		batch0[i] = kmalloc(alloc_size);

		if (!batch0[i]) {
			logging_log_error("Allocator assert failed - failed to allocate");
			panic(PANIC_STATE);
		}

		for (j = 0; j < alloc_size; j++) {
			batch0[i][j] = INTEGRITY_NUM;
		}

		alloc_size += 7 + alloc_size;
	}

	/* populate batch1 */
	alloc_size = 2;
	for (i = 0; i < NUM_BATCH; i++) {
		batch1[i] = kmalloc(alloc_size);

		if (!batch1[i]) {
			logging_log_error("Allocator assert failed - failed to allocate");
			panic(PANIC_STATE);
		}

		for (j = 0; j < alloc_size; j++) {
			batch1[i][j] = INTEGRITY_NUM;
		}

		alloc_size += 7 + alloc_size;
	}

	/* free and assert batch0 */
	alloc_size = 1;
	for (i = 0; i < NUM_BATCH; i++) {
		for (j = 0; j < alloc_size; j++) {
			if (batch0[i][j] != INTEGRITY_NUM) {
				logging_log_error("Allocator assert failed - inconsistent data");
				panic(PANIC_STATE);
			}
		}

		kfree(batch0[i]);
		alloc_size += 7 + alloc_size;
	}

	/* assert batch1 */
	alloc_size = 2;
	for (i = 0; i < NUM_BATCH; i++) {
		for (j = 0; j < alloc_size; j++) {
			if (batch1[i][j] != INTEGRITY_NUM) {
				logging_log_error("Allocator assert failed - inconsistent data");
				panic(PANIC_STATE);
			}
		}

		alloc_size += 7 + alloc_size;
	}

	/* populate batch2 */
	alloc_size = 3;
	for (i = 0; i < NUM_BATCH; i++) {
		batch2[i] = kmalloc(alloc_size);

		if (!batch2[i]) {
			logging_log_error("Allocator assert failed - failed to allocate");
			panic(PANIC_STATE);
		}

		for (j = 0; j < alloc_size; j++) {
			batch2[i][j] = INTEGRITY_NUM;
		}

		alloc_size += 7 + alloc_size;
	}

	/* populate batch0 */
	alloc_size = 7;
	for (i = 0; i < NUM_BATCH; i++) {
		batch0[i] = kmalloc(alloc_size);

		if (!batch0[i]) {
			logging_log_error("Allocator assert failed - failed to allocate");
			panic(PANIC_STATE);
		}

		for (j = 0; j < alloc_size; j++) {
			batch0[i][j] = INTEGRITY_NUM;
		}

		alloc_size += 7 + alloc_size;
	}

	/* free and assert batch2 */
	alloc_size = 3;
	for (i = 0; i < NUM_BATCH; i++) {
		for (j = 0; j < alloc_size; j++) {
			if (batch2[i][j] != INTEGRITY_NUM) {
				logging_log_error("Allocator assert failed - inconsistent data");
				panic(PANIC_STATE);
			}
		}

		kfree(batch2[i]);
		alloc_size += 7 + alloc_size;
	}

	/* free and assert batch1 */
	alloc_size = 2;
	for (i = 0; i < NUM_BATCH; i++) {
		for (j = 0; j < alloc_size; j++) {
			if (batch1[i][j] != INTEGRITY_NUM) {
				logging_log_error("Allocator assert failed - inconsistent data");
				panic(PANIC_STATE);
			}
		}

		kfree(batch1[i]);
		alloc_size += 7 + alloc_size;
	}

	/* free and assert batch0 */
	alloc_size = 7;
	for (i = 0; i < NUM_BATCH; i++) {
		for (j = 0; j < alloc_size; j++) {
			if (batch0[i][j] != INTEGRITY_NUM) {
				logging_log_error("Allocator assert failed - inconsistent data");
				panic(PANIC_STATE);
			}
		}

		kfree(batch0[i]);
		alloc_size += 7 + alloc_size;
	}

	logging_log_info("All allocation tests passed");
}
