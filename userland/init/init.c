/* init.c - userland entry point */
/* Copyright (C) 2025-2026  Ebrahim Aleem
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

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

extern char** environ;

static char* const shell_argv[] = {"/dev/dash", "/", 0};

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	struct stat statbuf;
	int fd;
	pid_t pid;
	int sts;

	// open tty

	if (stat("/dev/ttyS0", &statbuf) || (fd = open("/dev/ttyS0", O_RDWR)) < 0) {
		return EXIT_FAILURE;
	}

	if (fd != 0) {
		if (dup2(fd, 0) != 0) {
			return EXIT_FAILURE;
		}
	}

	if (dup2(fd, 1) != 1 || dup2(fd, 2) != 2) {
		return EXIT_FAILURE;
	}

	if (fd > 2) {
		if(close(fd)) {
			return EXIT_FAILURE;
		}
	}

	// open shell
	if (stat("/usr/bin/dash", &statbuf)) {
		return EXIT_FAILURE;
	}

	if ((pid = fork()) < 0) {
		return EXIT_FAILURE;
	}
	else if (pid == 0) {
		execve("/usr/bin/dash", shell_argv, environ);
		return EXIT_FAILURE;
	}

	// daemonize
	if (close(0)) {
		return EXIT_FAILURE;
	}	

	if (close(1)) {
		return EXIT_FAILURE;
	}	

	if (close(2)) {
		return EXIT_FAILURE;
	}	

	// reap orphans
	while (1) {
		while (1) {
			while ((pid = waitpid(-1, &sts, WNOHANG)) > 0);

			waitpid(-1, &sts, 0);
		}
	}
}
