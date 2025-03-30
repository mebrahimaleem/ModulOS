/* serial.c - serial port functions */
/* Copyright (C) 2025  Ebrahim Aleem
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

#ifndef CORE_SERIAL_C
#define CORE_SERIAL_C

#include <core/atomic.h>
#include <core/portio.h>
#include <core/utils.h>
#include <core/serial.h>

#define COM1_BASE 0x3f8
#define COM2_BASE 0x2f8

#define BUF_OFF   0x0
#define EIR_OFF   0x1
#define FCR_OFF   0x2
#define LCR_OFF   0x3
#define MCR_OFF   0x4
#define SCR_OFF   0x7

#define LP_TEST   0x9e

#define DIV_LATCH 0x80
#define LCR_8N1   0x3
#define FCR_RESET 0xc7
#define MCR_RESET 0x3
#define MCR_LOOP  0x12

uint8_t com_sts; //each bit represents a com

uint16_t COM3_BASE;
uint16_t COM4_BASE;
uint16_t COM5_BASE;
uint16_t COM6_BASE;
uint16_t COM7_BASE;
uint16_t COM8_BASE;

StaticMutexHandle serialMutex[8];

void serialinit(void) {
  com_sts = 0;

  uint8_t i;
  for (i = 0; i < 8; i++) {
    serialMutex[i] = kcreateStaticMutex();
  }

  // first setup com1
  outb(COM1_BASE + SCR_OFF, LP_TEST);
  iowait();
  if (inb(COM1_BASE + SCR_OFF) == LP_TEST) {
    // disable interrupts
    outb(COM1_BASE + EIR_OFF, 0);

    // set baud rate to 38400
    outb(COM1_BASE + LCR_OFF, DIV_LATCH);
    outb(COM1_BASE + BUF_OFF, 3);
    outb(COM1_BASE + EIR_OFF, 0);

    // set lcr to 8N1
    outb(COM1_BASE + LCR_OFF, LCR_8N1);

    // enable fifo
    outb(COM1_BASE + FCR_OFF, FCR_RESET);

    // set RST and DSR
    outb(COM1_BASE + MCR_OFF, MCR_RESET);

    // enter loopback mode
    outb(COM1_BASE + MCR_OFF, MCR_LOOP);

    // test serial port
    outb(COM1_BASE + BUF_OFF, LP_TEST);
    if (inb(COM1_BASE + BUF_OFF) == LP_TEST) {
      com_sts |= SERIAL1;

      // leave loopback mode
      outb(COM1_BASE + MCR_OFF, MCR_RESET);      
    }
  }

  // next com2
  outb(COM2_BASE + SCR_OFF, LP_TEST);
  iowait();
  if (inb(COM2_BASE + SCR_OFF) == LP_TEST) {
    // disable interrupts
    outb(COM2_BASE + EIR_OFF, 0);

    // set baud rate to 38400
    outb(COM2_BASE + LCR_OFF, DIV_LATCH);
    outb(COM2_BASE + BUF_OFF, 3);
    outb(COM2_BASE + EIR_OFF, 0);

    // set lcr to 8N1
    outb(COM2_BASE + LCR_OFF, LCR_8N1);

    // enable fifo
    outb(COM2_BASE + FCR_OFF, FCR_RESET);

    // set RST and DSR
    outb(COM2_BASE + MCR_OFF, MCR_RESET);

    // enter loopback mode
    outb(COM2_BASE + MCR_OFF, MCR_LOOP);

    // test serial port
    outb(COM2_BASE + BUF_OFF, LP_TEST);
    if (inb(COM2_BASE + BUF_OFF) == LP_TEST) {
      com_sts |= SERIAL2;

      // leave loopback mode
      outb(COM2_BASE + MCR_OFF, MCR_RESET);      
    }
  }
}

void serialWriteStr(uint8_t com, const char* str) {
  uint16_t addr;
  StaticMutexHandle handle;
  switch (com) {
    case SERIAL1:
      handle = serialMutex[0];
      addr = COM1_BASE;
      break;
    case SERIAL2:
      handle = serialMutex[1];
      addr = COM2_BASE;
      break;
    case SERIAL3:
      handle = serialMutex[2];
      addr = COM3_BASE;
      break;
    case SERIAL4:
      handle = serialMutex[3];
      addr = COM4_BASE;
      break;
    case SERIAL5:
      handle = serialMutex[4];
      addr = COM5_BASE;
      break;
    case SERIAL6:
      handle = serialMutex[5];
      addr = COM6_BASE;
      break;
    case SERIAL7:
      handle = serialMutex[6];
      addr = COM7_BASE;
      break;
    case SERIAL8:
      handle = serialMutex[7];
      addr = COM8_BASE;
      break;
    default:
      return;
  }
  // check if serial is configured
  if ((com_sts & com) == com) {
    //TODO: implement receiving over serial
    kacquireStaticMutex(handle);

    for (const char* c = str; *c != 0; c++) {
      outb(addr, (uint8_t)*c);
    }

    kreleaseStaticMutex(handle);
  }
}

void serialPrintf(uint8_t com, const char* fmt, ...) {
	va_list va;
	va_start(va, fmt);
	serialVprintf(com, fmt, va);
}

void serialVprintf(uint8_t com, const char* fmt, va_list va) {
	char* buf;
	formatstr(fmt, &buf, va);
	serialWriteStr(com, buf);
}

#endif /* CORE_SERIAL_C */
