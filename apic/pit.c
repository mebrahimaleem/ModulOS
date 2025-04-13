/* pit.c - PIT ISR */
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

#ifndef APIC_PIT_C
#define APIC_PIT_C

#include <stdint.h>

#include <core/portio.h>
#include <core/memory.h>
#include <core/threads.h>

#include <apic/ioapic.h>
#include <apic/isr.h>
#include <apic/pit.h>

#define PIT_DATA_ADDR	0x40
#define PIT_CMD_ADDR	0x43

#define PIT_BINARY		0x00
#define PIT_RATE			0x04
#define PIT_LOHI			0x30
#define PIT_CHN0			0x00

#define RELOAD_LO			0x9C
#define RELOAD_HI			0x2E

#define PIT_PERIOD_MS	10

struct PIT_deadline* pit_bot;

mutex_t pit_mutex;
uint64_t tracked_time;

void apic_initpit() {
	pit_bot = 0;

	pit_mutex = kcreateMutex();

	/* configure PIT */
	outb(PIT_CMD_ADDR, PIT_BINARY | PIT_RATE | PIT_LOHI | PIT_CHN0);
	outb(PIT_DATA_ADDR, RELOAD_LO);
	outb(PIT_DATA_ADDR, RELOAD_HI);

	apic_unmaskIrq(apic_translateGsi(IOAPIC_ISA_PIT));
	
		
	isr_registerCallback(apic_PIThandler, V_TO_CODE(apic_pitisr));
	tracked_time = 0;
}

void apic_addDeadline(pid_t PID, uint64_t delay) {
	kacquireMutex(pit_mutex);
	struct PIT_deadline* deadline = kmalloc(sizeof(struct PIT_deadline));
	
	deadline->PID = PID;
	deadline->deadline = delay + apic_pit_time();

	deadline->next = pit_bot;
	deadline->prev = 0;
	pit_bot = deadline;
	kreleaseMutex(pit_mutex);
}

inline uint64_t apic_pit_time() {
	return tracked_time;
}

void apic_PIThandler(uint64_t internal) {
	tracked_time += PIT_PERIOD_MS;

	kacquireMutex(pit_mutex);
	const uint64_t time = apic_pit_time();

	struct PIT_deadline* next;
	for (struct PIT_deadline* i = pit_bot; i != 0; i = next) {
		next = i->next;

		if (time >= i->deadline) {
			/* remove and send wakeup */
			if (i->prev != 0) {
				i->prev->next = i->next;
			}
			else {
				pit_bot = i->next;
			}
			thread_wakeup(i->PID);
			kfree(i);	
		}
	}
	kreleaseMutex(pit_mutex);
}

#endif /* APIC_PIT_C */
