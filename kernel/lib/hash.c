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

#define CRC32_ANSI_C0	0xFFFFFFFF
#define CRC32_ANSI_C1	0xEDB88320;


#define FNV64_1A_C0		0xcbf29ce484222325
#define FNV64_1A_C1		0x100000001b3

uint8_t hash_byte_sum(const void* ptr, size_t c) {
	const uint8_t* _ptr = ptr;
	uint8_t sum = 0;
	for (; c > 0; c--) {
		sum += *(_ptr++);
	}

	return sum;
}

uint32_t crc32_ansi(const void* data, size_t length) {
	uint32_t crc = CRC32_ANSI_C0;
	const uint8_t *p = (const uint8_t*)data;
	uint8_t i;

	while (length--) {
		crc ^= *p++;

		for (i = 0; i < 8; i++) {
			if (crc & 1) {
				crc = (crc >> 1) ^ CRC32_ANSI_C1;
			}
			else {
				crc >>= 1;
			}
		}
	}

	return crc ^ CRC32_ANSI_C0;
}

uint64_t fnv64_1a(const void* data, size_t length) {
	const uint8_t* bytes = data;
	uint64_t fnv64 = FNV64_1A_C0;

	for (size_t i = 0; i < length; i++) {
		fnv64 ^= *bytes;
		fnv64 *= FNV64_1A_C1;
		bytes++;
	}

	return fnv64;
}
