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

#define TTY_RING_MASK	0x3FFF

#define MASK(i)				((i) & TTY_RING_MASK)
#define EMPTY(r, w)		(r == w)
#define FULL(r, w)		(MASK(w + 1) == r)

#ifdef SERIAL
#include <drivers/serial/serial.h>
#endif /* SERIAL */

typedef void (*tty_write_t)(uint8_t byte);

struct tty_handle_t {
	tty_write_t writer;
	uint8_t* read_buffer;
	volatile uint16_t write_index;
	volatile uint16_t read_index;
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
	lock_init(&com1.lock);

	com2.writer = serial_write_com2;
	com2.read_buffer = (uint8_t*)paging_ident(mm_alloc_p(TTY_READ_BUFFER_SIZE));
	com2.write_index = 0;
	com2.read_index = 0;
	lock_init(&com2.lock);
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
		while (EMPTY(tty->read_index, tty->write_index)) {
			cpu_pause();
		}

		lock_acquire(&tty->lock);
		while (count && !EMPTY(tty->read_index, tty->write_index)) {
			*write = tty->read_buffer[MASK(tty->read_index)];

			tty->read_index++;
			write++;
			count--;

			tty->read_index &= TTY_RING_MASK;
		}
		lock_release(&tty->lock);
	}
}

uint8_t tty_queue_read(struct tty_handle_t* tty, uint8_t byte) {
	if (FULL(tty->read_index, tty->write_index)) {
		return 0;
	}

	tty->read_buffer[MASK(tty->write_index)] = byte;

	tty->write_index++;
	tty->write_index &= TTY_RING_MASK;

	return 1;
}

void tty_write(struct tty_handle_t* tty, void* buffer, size_t count) {
	for (size_t i = 0; i < count; i++) {
		tty->writer(((uint8_t*)buffer)[i]);
	}
}
