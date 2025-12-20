/* serial_str.c - serial string io */
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

#include <stdint.h>
#include <stdarg.h>

#include <serial/serial_print.h>
#include <serial/serial.h>

static const char low_charset[] =
	{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
static const char upper_charset[] =
	{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

struct num_printer_t {
	int base;
	const char* charset;
	void (*printer)(uint8_t);
};

#define _PRINT_UINT(s) \
	static void _print_uint##s(uint##s##_t num, struct num_printer_t* meta) { \
		if (!num) return; \
		const int res = num % meta->base; \
		_print_uint##s(num / meta->base, meta); \
		meta->printer(meta->charset[res]); \
	}

#define PRINT_INT(s) \
	static void print_int##s(int##s##_t num, int base, const char* charset, void (*printer)(uint8_t)) { \
		struct num_printer_t meta; \
		meta.base = base; \
		meta.charset = charset; \
		meta.printer = printer; \
		if (num > 0) { \
			_print_uint##s(num, &meta); \
		} \
		else if (num < 0) { \
			printer('-'); \
			_print_uint##s(-num, &meta); \
		} \
		else { \
			printer('0'); \
		} \
	}

#define PRINT_UINT(s) \
	static void print_uint##s(uint##s##_t num, int base, const char* charset, void (*printer)(uint8_t)) { \
		struct num_printer_t meta; \
		meta.base = base; \
		meta.charset = charset; \
		meta.printer = printer; \
		if (num) { \
			_print_uint##s(num, &meta); \
		} \
		else { \
			printer('0'); \
		} \
	}

#define CHOSE_SIZE(m, b, c) \
					s++; \
					switch (*s) { \
						case '6': \
							s++; \
							print_##m##64(va_arg(args, m##64_t), b, c##_charset, printer); \
							break; \
						case '3': \
							s++; \
							print_##m##32(va_arg(args, m##32_t), b, c##_charset, printer); \
							break; \
						default: \
							break; \
					}

_PRINT_UINT(64)
_PRINT_UINT(32)

PRINT_INT(64)
PRINT_INT(32)

PRINT_UINT(64)
PRINT_UINT(32)

static void serial_print(const char* s, void (*printer)(uint8_t)) {
	for (; *s != 0; s++) {
		printer(*s);
	}
}

static void serial_printf(const char* s, void (*printer)(uint8_t), va_list args) {
	for (; *s != 0; s++) {
		switch (*s) {
			case '%':
				s++;
				switch (*s) {
					case 'd':
						CHOSE_SIZE(int, 10, low)
						break;
					case 'u':
						CHOSE_SIZE(uint, 10, low)
						break;
					case 'x':
						CHOSE_SIZE(uint, 16, low)
						break;
					case 'X':
						CHOSE_SIZE(uint, 16, upper)
						break;
					case 's':
						serial_print(va_arg(args, const char*), printer);
						break;
					case '%':
						printer('%');
						break;
					default:
						break;
				}
				break;
			default:
				printer(*s);
				break;
		}
	}
}

void serial_print_com1(const char* s) {
	serial_print(s, serial_write_com1);
}

void serial_print_com2(const char* s) {
	serial_print(s, serial_write_com2);
}

void serial_printf_com1(const char* s, ...) {
	va_list args;
	va_start(args, s);
	serial_printf(s, serial_write_com1, args);
	va_end(args);
}

void serial_printf_com2(const char* s, ...) {
	va_list args;
	va_start(args, s);
	serial_printf(s, serial_write_com2, args);
	va_end(args);
}
