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
#include <stdlib.h>

int main(int argc, char** argv) {
	printf("%s\n", argv[0]);
	if (argc > 1) {
		printf("%s ", argv[1]);
	}
	printf("Shell\n");

	FILE* f = fopen(argv[0], "r");

	if (!f) {
		perror("Failed top open file");
	}

	static char buffer[4];

	ssize_t read = fread(buffer, 1, sizeof(buffer), f);
	if (read != 4) {
		fprintf(stderr, "Failed to read file\n");
		return EXIT_FAILURE;
	}

	printf("%3s\n", &buffer[1]);

	fclose(f);

	return EXIT_SUCCESS;
}
