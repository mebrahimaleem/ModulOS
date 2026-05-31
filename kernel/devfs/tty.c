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
#include <core/signal.h>

#include <lib/kstrcmp.h>
#include <lib/kmemcpy.h>
#include <lib/kstrlen.h>

#ifdef SERIAL
#include <drivers/serial/interrupts.h>
#endif /* SERIAL */

#define TTY_RING_MASK	0x3FFF

#define MASK(i)				((i) & TTY_RING_MASK)
#define EMPTY(r, w)		(r == w)
#define FULL(r, w)		(MASK(w + 1) == r)

#ifdef SERIAL
#include <drivers/serial/serial.h>
#endif /* SERIAL */

#define BS		0x08
#define DEL		0x7f
#define ESC		0x1b

#define F_ESC	0x01
#define F_CSI	0x02

#define MAX_CSI_LEN		32

typedef void (*tty_write_t)(uint8_t byte);

struct tty_handle_t {
	tty_write_t writer;
	uint8_t* read_buffer;
	uint16_t write_index;
	uint16_t read_index;
	uint16_t ls_index;
	uint16_t le_index;
	struct signal_wait_t* signal;
	enum {
		TTY_MODE_COOKED,
		TTY_MODE_RAW
	} tty_mode;
	uint8_t lock;
	uint8_t flg;
	uint8_t csi[MAX_CSI_LEN];
	uint8_t csi_index;
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
	com1.ls_index = 0;
	com1.le_index = 0;
	com1.signal = signal_wait_alloc();
	com1.tty_mode = TTY_MODE_COOKED;
	com1.flg = 0;
	lock_init(&com1.lock);

	com2.writer = serial_write_com2;
	com2.read_buffer = (uint8_t*)paging_ident(mm_alloc_p(TTY_READ_BUFFER_SIZE));
	com2.write_index = 0;
	com2.read_index = 0;
	com2.ls_index = 0;
	com2.le_index = 0;
	com2.signal = signal_wait_alloc();
	com2.tty_mode = TTY_MODE_COOKED;
	com2.flg = 0;
	lock_init(&com2.lock);

	serial_init_interrupts();
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

struct tty_handle_t* tty_open(const char* name) {
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

size_t tty_read(struct tty_handle_t* tty, void* buffer, size_t count) {
	size_t read = 0;
	uint8_t* write = buffer;

	while (count) {
		while (EMPTY(tty->read_index, tty->ls_index)) {
			signal_wait(tty->signal);
		}

		lock_acquire(&tty->lock);

		while (count && !EMPTY(tty->read_index, tty->ls_index)) {
			*write = tty->read_buffer[MASK(tty->read_index)];

			tty->read_index++;
			count--;
			read++;

			if (*write == '\n' && tty->tty_mode == TTY_MODE_COOKED) {
				// cooked mode must return early on newline
				count = 0;
			}

			write++;

			tty->read_index &= TTY_RING_MASK;
		}

		lock_release(&tty->lock);
	}

	return read;
}

void tty_write(struct tty_handle_t* tty, const void* buffer, size_t count) {
	for (size_t i = 0; i < count; i++) {
		uint8_t val = ((const uint8_t*)buffer)[i];
		switch (val) {
			case '\n':
				tty->writer('\r');
				__attribute__((fallthrough));
			default:
				tty->writer(((const uint8_t*)buffer)[i]);
				break;
		}
	}
}

uint8_t tty_queue_read(struct tty_handle_t* tty, uint8_t byte) {
	if (tty->tty_mode == TTY_MODE_COOKED) {
		if (byte == 0 || FULL(tty->read_index, tty->write_index)) {
			return 0;
		}

		if (byte == ESC) {
			tty->flg |= F_ESC;
			return 1;
		}

		if (byte == '[' && (tty->flg & F_ESC)) {
			tty->flg |= F_CSI;
			tty->csi_index = 0;
			return 1;
		}

		if ((tty->flg & F_CSI)) {
			if (tty->csi_index == MAX_CSI_LEN) {
				tty->flg ^= F_CSI;
				return 0;
			}
			tty->csi[tty->csi_index++] = byte;

			if (byte >= 0x40 && byte < 0x80) {
				tty->flg ^= F_CSI;

				// TODO: handle other csi bytes
				switch (byte) {
					case 'C':
						if (tty->write_index != tty->le_index) {
							tty->write_index++;
							tty->write_index &= TTY_RING_MASK;

							tty->writer(ESC);
							tty->writer('[');
							tty->writer('C');
						}
						break;
					case 'D':
						if (tty->write_index != tty->ls_index) {
							tty->write_index--;
							tty->write_index &= TTY_RING_MASK;

							tty->writer(ESC);
							tty->writer('[');
							tty->writer('D');
						}
						break;
				}
			}
			return 1;
		}

		// echoback
		switch (byte) {
			case BS:
			case DEL:
				if(tty->ls_index != tty->write_index) {
					tty->writer(BS);
					tty->writer(' ');
					tty->writer(BS);
				}
				byte = BS;
				break;
			case '\r':
				byte = '\n';
				__attribute__((fallthrough));
			case '\n':
				tty->writer('\r');
				tty->writer('\n');
				break;
			default:
				tty->writer(byte);
				break;
		}

		if ((tty->flg & F_ESC) && byte != ESC) {
			tty->flg ^= F_ESC;
		}

		uint8_t ret;

		switch (byte) {
			case BS:
				if(tty->ls_index == tty->write_index) {
					ret = 0;
					break;
				}

				tty->write_index--;
				tty->write_index &= TTY_RING_MASK;

				tty->read_buffer[tty->write_index] = ' ';
				ret = 1;
				break;
			case '\n':
				tty->read_buffer[tty->le_index] = '\n';
				tty->write_index = MASK(tty->le_index + 1);
				tty->ls_index = tty->le_index = tty->write_index;
				signal_awake(tty->signal);
				ret = 1;
				break;
			default:
				tty->read_buffer[MASK(tty->write_index)] = byte;

				if (tty->le_index == tty->write_index) {
					tty->le_index++;
					tty->le_index &= TTY_RING_MASK;
				}

				tty->write_index++;
				tty->write_index &= TTY_RING_MASK;
				ret = 1;
				break;
		}

		return ret;
	}

	return 0;
}
