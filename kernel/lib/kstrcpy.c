/* kstrcpy.c - kernel string copy implementation */
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

#include <lib/kstrcpy.h>

char* kstrcpy(char* dest, const char* src) {
	while (*src) {
		*dest = *src;
		src++;
		dest++;
	}

	*dest = 0;
	return dest + 1;
}

char* kstrcpy_no_null(char* dest, const char* src) {
	while (*src) {
		*dest = *src;
		src++;
		dest++;
	}

	return dest;
}

char* kstrncpy(char* dest, const char* src, size_t len) {
	if (!len) {
		return dest;
	}

	while (*src && --len) {
		*dest = *src;
		src++;
		dest++;
	}

	*dest = 0;
	return dest + 1;
}
