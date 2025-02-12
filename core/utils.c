/* utils.c - kernel utils */
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

#ifndef CORE_UTILS_C
#define CORE_UTILS_C

#include <stdint.h>

#include <core/utils.h>

void kcopy(void* src, void* dst, uint64_t len) {
	uint64_t i;
	for (i = 0; i < len; i++) { /* TODO: manually optimize with asm rep movsb/s/l/q */
		*((uint8_t*)dst + i) = *((uint8_t*)src + i);
	}
}

void kfill(void* dst, uint64_t len, uint8_t val) {
	uint64_t i;
	for (i = 0; i < len; i++) { /* TODO: manually optimize with asm rep stosb/s/l/q */
		*((uint8_t*)dst + i) = val;
	}
}

uint64_t intToString(uint64_t num, uint8_t base, char* str, uint64_t strlen) {
	if (num == 0 && strlen > 0){
		*(str + strlen) = '0';
		return 1;
	}
	uint64_t i = 0;
	for (char* c = str + strlen; c >= str; c--) {
		*c = (char)(num % base);

		if (*c > 9) {
			// alpha
			*c += 'A' - 10;
		}
		else {
			// numeric
			*c += '0';
		}

		num /= base;

		i++;
		if (num == 0) break;
	}

	return i;
}

void kstrcpy(const char* from, char* to) {
	for (int i = 0; from[i] != 0; i++) {
		to[i] = from[i]; //TODO: manually optimize with asm
	}
}

#endif /* CORE_UTILS_C */
