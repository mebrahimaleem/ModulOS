/* logging.c - logging implementation */
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

#include <core/logging.h>

#define MAX_LOGGER_OUT	4

static void (*loggers[MAX_LOGGER_OUT])(enum log_severity_t, const char* format, va_list args);
static uint64_t last_logger;

static void log(enum log_severity_t severity, const char* format, va_list args) {
	for (uint64_t i = 0 ; i < last_logger; i++) {
		loggers[i](severity, format, args);
	}
}

void logging_init(void) {
	last_logger = 0;
}

void logging_register(void (*logger)(enum log_severity_t, const char* format, va_list args)) {
	loggers[last_logger++] = logger;
}

void _logging_log_debug(const char* format, ...) {
	va_list args;
	va_start(args, format);
	log(SEVERITY_DBG, format, args);
	va_end(args);
}

void logging_log_info(const char* format, ...) {
	va_list args;
	va_start(args, format);
	log(SEVERITY_INF, format, args);
	va_end(args);
}

void logging_log_warning(const char* format, ...) {
	va_list args;
	va_start(args, format);
	log(SEVERITY_WRN, format, args);
	va_end(args);
}

void logging_log_error(const char* format, ...) {
	va_list args;
	va_start(args, format);
	log(SEVERITY_ERR, format, args);
	va_end(args);
}
