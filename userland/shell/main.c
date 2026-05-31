/* main.c - ModulOS shell */
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

#include <stdio.h>

int main(int argc, char** argv) {
	for (int i = 0; i < argc; i++) {
		printf("%s ", argv[i]);
	}

	printf("\n");

	char buf[256];

	while (1) {
		printf("$");
		fflush(stdout);
		fgets(buf, 256, stdin);
		printf(">>%s", buf);
	}
}
