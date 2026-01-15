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

#define FLG_LL	0x1
#define FLG_PD	0x2
#define FLG_LM	0x4

static const char low_charset[] =
	{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
static const char upper_charset[] =
	{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

struct num_printer_t {
	int base;
	const char* charset;
	void (*printer)(uint8_t);
};

static void serial_print(const char* s, void (*printer)(uint8_t));

#define _PRINT_UINT(s) \
	static void _print_uint##s(uint##s##_t num, struct num_printer_t* meta, uint8_t flg, uint64_t lim) { \
		if (!num || (!lim && (flg & FLG_LM) == FLG_LM)) return; \
		const int res = (int)(num % (uint64_t)meta->base); \
		_print_uint##s(num / (uint64_t)meta->base, meta, flg, --lim); \
		meta->printer((uint8_t)meta->charset[res]); \
	}

#define PRINT_INT(s) \
	static void print_int##s(int##s##_t num, int base, const char* charset, void (*printer)(uint8_t), uint8_t flg, uint64_t lim) { \
		struct num_printer_t meta; \
		meta.base = base; \
		meta.charset = charset; \
		meta.printer = printer; \
		if (num > 0) { \
			_print_uint##s((uint##s##_t)num, &meta, flg, lim); \
		} \
		else if (num < 0) { \
			printer('-'); \
			_print_uint##s((uint##s##_t)(-num), &meta, flg, --lim); \
		} \
		else { \
			serial_print("0", printer); \
		} \
	}

#define PRINT_UINT(s) \
	static void print_uint##s(uint##s##_t num, int base, const char* charset, void (*printer)(uint8_t), uint8_t flg, uint64_t lim) { \
		struct num_printer_t meta; \
		meta.base = base; \
		meta.charset = charset; \
		meta.printer = printer; \
		if (num) { \
			_print_uint##s(num, &meta, flg, lim); \
		} \
		else { \
			serial_print("0", printer); \
		} \
	}

#define CHOSE_SIZE(m, b, c) \
					s++; \
					switch (*s) { \
						case '6': \
							s++; \
							print_##m##64(va_arg(args, m##64_t), b, c##_charset, printer, flg, width); \
							break; \
						case '3': \
							s++; \
							print_##m##32(va_arg(args, m##32_t), b, c##_charset, printer, flg, width); \
							break; \
						default: \
							if ((flg & FLG_LL) == FLG_LL) \
								print_##m##64(va_arg(args, m##64_t), b, c##_charset, printer, flg, width); \
							else \
								print_##m##32(va_arg(args, m##32_t), b, c##_charset, printer, flg, width); \
							s--; \
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
		printer((uint8_t)*s);
	}
}

static void serial_com12(uint8_t b) {
	serial_write_com1(b);
	serial_write_com2(b);
}

static void serial_printf(const char* s, void (*printer)(uint8_t), va_list args) {
	for (; *s != 0; s++) {
		switch (*s) {
			case '%':
				uint8_t flg = 0;
				uint64_t width = 0;
				uint64_t precision = 0;
next_specifier:
				s++;
				switch (*s) {
					case 'd':
						CHOSE_SIZE(int, 10, low)
						break;
					case 'u':
						CHOSE_SIZE(uint, 10, low)
						break;
					case 'p':
					case 'x':
						CHOSE_SIZE(uint, 16, low)
						break;
					case 'X':
						CHOSE_SIZE(uint, 16, upper)
						break;
					case 's':
						const char* str = va_arg(args, const char*);
						if ((flg & FLG_LM) == FLG_LM) {
							for (; width && *str; width--) {
								printer((uint8_t)*(str++));
								precision--;
							}
							if ((flg & FLG_PD) == FLG_PD) {
								for (; precision; precision--) {
									printer(' ');
								}
							}
						}
						else {
							serial_print(str, printer);
						}
						break;
					case 'l':
						flg |= FLG_LL;
						goto next_specifier;
					case '%':
						printer('%');
						break;
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
						if ((flg & FLG_PD) == FLG_PD)
							precision = precision * 10 + (uint64_t)*s - '0';
						else {
							flg |= FLG_LM;
							width = width * 10 + (uint64_t)*s - '0';
						}
						goto next_specifier;
					case '.':
						flg |= FLG_PD;
						goto next_specifier;
					default:
						goto next_specifier; //TODO: implement other specifiers
				}
				break;
			case '\n':
				// support either crlf or lf
				printer('\r');
				printer('\n');
				break;
			default:
				printer((uint8_t)*s);
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

void serial_log(enum log_severity_t severity, const char* s, va_list args) {
	switch (severity) {
		case SEVERITY_DBG:
			serial_print("DEBUG: ", serial_com12);
			serial_printf(s, serial_com12, args);
			serial_print("\r\n", serial_com12);
			break;
		case SEVERITY_INF:
			serial_print("INFO: ", serial_com12);
			serial_printf(s, serial_com12, args);
			serial_print("\r\n", serial_com12);
			break;
		case SEVERITY_WRN:
			serial_print("WARNING: ", serial_com12);
			serial_printf(s, serial_com12, args);
			serial_print("\r\n", serial_com12);
			break;
		case SEVERITY_ERR:
			serial_print("ERROR: ", serial_com12);
			serial_printf(s, serial_com12, args);
			serial_print("\r\n", serial_com12);
			break;
		default:
			serial_printf(s, serial_com12, args);
			break;
	}
}
