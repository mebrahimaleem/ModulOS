/* logging.h - logging interface */
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

#ifndef KERNEL_CORE_LOGGING_H
#define KERNEL_CORE_LOGGING_H

#include <stdarg.h>

enum log_severity_t {
	SEVERITY_DBG,
	SEVERITY_INF,
	SEVERITY_WRN,
	SEVERITY_ERR
};

extern void logging_init(void);

extern void logging_register(void (*logger)(enum log_severity_t, const char* format, va_list args));

#if defined(DEBUG) || defined(DEBUG_LOGGING)
#define logging_log_debug(...) _logging_log_debug(__VA_OPT__(__VA_ARGS__))
#else
#define logging_log_debug(...) ((void)0)
#endif

extern void _logging_log_debug(const char* format, ...);
extern void logging_log_info(const char* format, ...);
extern void logging_log_warning(const char* format, ...);
extern void logging_log_error(const char* format, ...);

#endif /* KERNEL_CORE_LOGGING_H */
