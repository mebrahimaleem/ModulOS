/* tty.h - tty device interface */
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

#ifndef KERNEL_DEVFS_TTY_H
#define KERNEL_DEVFS_TTY_H

#include <stdint.h>
#include <stddef.h>

#define TTY_READ_BUFFER_SIZE		0x4000

struct tty_handle_t;

extern void tty_init(void);

#ifdef SERIAL
extern struct tty_handle_t* tty_com1(void);
extern struct tty_handle_t* tty_com2(void);
#endif /* SERIAL */

extern struct tty_handle_t* tty_open(char* name);
extern void tty_close(struct tty_handle_t* tty);
extern size_t tty_read(struct tty_handle_t* tty, void* buffer, size_t count);
extern uint8_t tty_queue_read(struct tty_handle_t* tty, uint8_t byte);
extern void tty_write(struct tty_handle_t* tty, void* buffer, size_t count);

#endif /* KERNEL_DEVFS_TTY_H */
