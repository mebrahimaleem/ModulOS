/* hash.h - hashing functions */
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

#include <stdint.h>
#include <stddef.h>

#include <lib/hash.h>

uint8_t hash_byte_sum(const void* ptr, size_t c) {
	const uint8_t* _ptr = ptr;
	uint8_t sum = 0;
	for (; c > 0; c--) {
		sum += *(_ptr++);
	}

	return sum;
}

uint32_t crc32_ansi(const void* data, size_t length) {
	uint32_t crc = 0xFFFFFFFF;
	const uint8_t *p = (const uint8_t*)data;
	uint8_t i;

	while (length--) {
		crc ^= *p++;

		for (i = 0; i < 8; i++) {
			if (crc & 1) {
				crc = (crc >> 1) ^ 0xEDB88320;
			}
			else {
				crc >>= 1;
			}
		}
	}

	return crc ^ 0xFFFFFFFF;
}
