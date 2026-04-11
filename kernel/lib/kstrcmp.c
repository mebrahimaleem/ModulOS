/* kstrcmp.c - kernel string compare implementation */
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

#include <lib/kstrcmp.h>

int kstrcmp(const char* s1, const char* s2) {
	while (*s1 && *s2 && *s1 == *s2) {
		s1++;
		s2++;
	}

	return (int)(*s1 - *s2);
}
