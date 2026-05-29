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
	(void)argc;
	(void)argv;

	static char buffer[8];

	printf("Shell\n");

	FILE* f = fopen("/test.txt", "r");
	if (f == 0) {
		perror("Failed to open file");
		return EXIT_FAILURE;
	}

	ssize_t bytes_read = fread(buffer, 1, 8, f);
	if (bytes_read == -1) {
		perror("Failed to read file");
		return EXIT_FAILURE;
	}

	fclose(f);

	printf("Bytes read: %lu\nContent: %s", bytes_read, buffer);

	static char input_buf[64];
	printf("Enter file name:\n");
	scanf("%s", input_buf);

	f = fopen(input_buf, "a");

	if (f == 0) {
		perror("Failed to open file");
		return EXIT_FAILURE;
	}

	if (fwrite(buffer, 1, bytes_read, f) != bytes_read) {
		perror("Failed to write to file");
		return EXIT_FAILURE;
	}

	fclose(f);

	printf("Done\n");

	return EXIT_SUCCESS;
}
