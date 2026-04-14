/* tty.c - tty device handler */
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

#include <stdint.h>
#include <stddef.h>

#include <devfs/tty.h>

#include <core/alloc.h>
#include <core/lock.h>
#include <core/cpu_instr.h>
#include <core/mm.h>
#include <core/paging.h>

#include <lib/kstrcmp.h>
#include <lib/kmemcpy.h>
#include <lib/kstrlen.h>

#ifdef SERIAL
#include <drivers/serial/serial.h>
#endif /* SERIAL */

typedef void (*tty_write_t)(uint8_t byte);

struct tty_handle_t {
	tty_write_t writer;
	uint8_t* read_buffer;
	uint16_t write_index;
	uint16_t read_index;
	uint8_t bytes_ready;
	uint8_t lock;
};

#ifdef SERIAL

static struct tty_handle_t com1;
static struct tty_handle_t com2;

#endif /* SERIAL */

void tty_init(void) {
#ifdef SERIAL
	com1.writer = serial_write_com1;
	com1.read_buffer = (uint8_t*)paging_ident(mm_alloc_p(TTY_READ_BUFFER_SIZE));
	com1.write_index = 0;
	com1.read_index = 0;
	com1.bytes_ready = 0;
	lock_init(&com1.lock);

	com2.writer = serial_write_com1;
	com2.read_buffer = (uint8_t*)paging_ident(mm_alloc_p(TTY_READ_BUFFER_SIZE));
	com2.write_index = 0;
	com2.read_index = 0;
	com2.bytes_ready = 0;
	lock_init(&com1.lock);
#endif /* SERIAL */
}

#ifdef SERIAL
struct tty_handle_t* tty_com1(void) {
	return &com1;
}

struct tty_handle_t* tty_com2(void) {
	return &com2;
}
#endif /* SERIAL */

struct tty_handle_t* tty_open(char* name) {
#ifdef SERIAL
	if (!kstrcmp(name, "S0")) {
		// COM1
		return &com1;
	}
	else if (!kstrcmp(name, "S1")) {
		// COM2
		return &com2;
	}
#endif /* SERIAL */

	return 0;
}

void tty_read(struct tty_handle_t* tty, void* buffer, size_t count) {
	uint8_t* write = buffer;

	while (count) {
		while (!tty->bytes_ready) {
			cpu_pause();
		}

		lock_acquire(&tty->lock);
		if (tty->bytes_ready) {
			do {
				*write = tty->read_buffer[tty->read_index];

				tty->read_index++;
				write++;
				count--;

				if (tty->read_index == TTY_READ_BUFFER_SIZE) {
					tty->read_index = 0;
				}
			} while (tty->read_index != tty->write_index && count);
		}

		tty->bytes_ready = tty->read_index != tty->write_index;
		lock_release(&tty->lock);
	}
}

uint8_t tty_queue_read(struct tty_handle_t* tty, uint8_t byte) {
	uint8_t ret = 0;

	lock_acquire(&tty->lock);
	if (!tty->bytes_ready || tty->read_index != tty->write_index) {
		tty->read_buffer[tty->write_index] = byte;
		ret = 1;

		tty->write_index++;

		if (tty->write_index == TTY_READ_BUFFER_SIZE) {
			tty->write_index = 0;
		}

		tty->bytes_ready = 1;
	}
	lock_release(&tty->lock);

	return ret;
}

void tty_write(struct tty_handle_t* tty, void* buffer, size_t count) {
	for (size_t i = 0; i < count; i++) {
		tty->writer(((uint8_t*)buffer)[i]);
	}
}
