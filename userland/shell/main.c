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

extern char** environ;

extern int fork();
extern void execve(const char* path, char* const argv[], char* const envp[]);

int main(int argc, char** argv) {
	for (int i = 0; i < argc; i++) {
		printf("%s\n", argv[i]);
	}

	for (int i = 0; environ[i]; i++) {
		printf("%s\n", environ[i]);
	}

	if (fork()) {
		printf("Forked. Exiting...\n");
	}
	else {
		printf("Self executing...\n");
		char* const a[] = {argv[0], "Test", 0};
		char* const e[] = {"Callee=US", 0};
		execve(argv[0], a, e);
		printf("Fatal\n");
	}

	return EXIT_SUCCESS;
}
