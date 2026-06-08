/* kstrcpy.h - kernel string copy interface */
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

#ifndef KERNEL_LIB_KSTRCPY_H
#define KERNEL_LIB_KSTRCPY_H

#include <stddef.h>

extern char* kstrcpy(char* dest, const char* src);

extern char* kstpcpy(char* dest, const char* src);

extern char* kstrcpy_no_null(char* dest, const char* src);

extern char* kstrncpy(char* dest, const char* src, size_t len);

#endif /* KERNEL_LIB_KSTRCPY_H */

