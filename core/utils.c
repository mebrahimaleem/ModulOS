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

#include <core/memory.h>
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

uint64_t uintToString(uint64_t num, uint8_t base, char* str, uint64_t strlen) {
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

uint64_t intToString(int64_t num, uint8_t base, char* str, uint64_t strlen) {
	uint64_t abs = (uint64_t)(num < 0 ? -num : num);
	uint64_t ret = uintToString(abs, base, str, strlen);

	if (num < 0) {
		str[strlen - ret] = '-';
		return ret+1;
	}
	return ret;
}

void kstrcpy(const char* from, char* to) {
	for (int i = 0; from[i] != 0; i++) {
		to[i] = from[i]; //TODO: manually optimize with asm
	}
}

uint64_t kstrlen(const char* str) {
	uint64_t len = 0;
	while (*str != 0) {
		len++;
		str++;
	}

	return len;
}

uint64_t formatstr(const char* str, char** dest, va_list va) {
	uint64_t printed = 0;
	const char* format = str;
	uint64_t maxlen = 64;
	*dest = kmalloc(kheap_private, sizeof(char) * maxlen);

	char buf[66];
	buf[65] = 0;

	const char* t0;
	int64_t t1;
	uint64_t t2;
	uint64_t t4;
	char t3;

	const char* append;

	while (*format != 0) {
		if (*format == '%') {
			format++;
			uint64_t width = 0;
			uint64_t precision = (uint64_t)-1;
			uint64_t justify = 0;

			if (*format == '-') {
				justify = 1;
				format++;
			}

			while (*format >= '0' && *format <= '9') {
				width = width * 10 + (uint64_t)(*format - '0');
				format++;
			}

			if (*format == '.') {
				format++;
				precision = 0;
				while (*format >= '0' && *format <= '9') {
					precision = precision * 10 + (uint64_t)(*format - '0');
					format++;
				}
			}

			uint8_t l = 0;
			if (*format == 'l') {
				format++;
				l++;
			}

			if (*format == 'l') {
				format++;
				l++;
			}

			switch (*format) { //TODO: fix printing error here and add mmap support for acpica
				case 's':
					t0 = va_arg(va, char*);
					append = t0;
					break;
				case 'd':
					t1 = l > 0 ? va_arg(va, int64_t) : va_arg(va, int32_t);
					t4 = intToString(t1, 10, &buf[0], 64);
					append = &buf[0] + 65 - t4;
					break;
				case 'u':
					t2 = l > 0 ? va_arg(va, uint64_t) : va_arg(va, uint32_t);
					t4 = 10;
					goto t2ToAppend;
				case 'x': //TODO: support lower and upper case
				case 'X':
					t2 = l > 0 ? va_arg(va, uint64_t) : va_arg(va, uint32_t);
					t4 = 16;
					goto t2ToAppend;
				case 'o':
					t2 = l > 0 ? va_arg(va, uint64_t) : va_arg(va, uint32_t);
					t4 = 8;
					goto t2ToAppend;
				case 'c':
					t3 = (char)va_arg(va, int);
					buf[0] = t3;
					buf[1] = 0;
					append = &buf[0];
					break;
				case '%':
					buf[0] = '%';
					buf[1] = 0;
					append = &buf[0];
					break;
				case 'f':
				case 'e':
				case 'E':
				case 'g':
				case 'G':
				case 'p': //TODO: add support
				default:
					break;
				t2ToAppend:
					t4 = uintToString(t2, (uint8_t)t4, &buf[0], 64);
					append = &buf[0] + 65 - t4;
					break;
			}

			t2 = kstrlen(append);
			for(t4 = 0; t4 < t2 && precision > 0; t4++) {
				if (printed == maxlen) {
					maxlen *= 2;
					*dest = krealloc(kheap_private, *dest, sizeof(char) * maxlen);
				}
				(*dest)[printed] = append[t4];
				printed++;
				precision--;
			}

			//TODO: implement justification
			//TODO: implement width
		}

		else {
			if (printed == maxlen) {
				maxlen *= 2;
				*dest = krealloc(kheap_private, *dest, sizeof(char) * maxlen);
			}
			(*dest)[printed] = *format;
			printed++;
		}

		format++;
	}

	if (printed == maxlen) {
		*dest = krealloc(kheap_private, *dest, sizeof(char) * maxlen + 1);
	}

	(*dest)[printed] = 0;

	return printed;
}

#endif /* CORE_UTILS_C */
