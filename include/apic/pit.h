/* pit.h - PIT ISR */
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

#ifndef APIC_PIT_H
#define APIC_PIT_H

#include <core/threads.h>

extern mutex_t pit_mutex;

struct PIT_deadline {
	pid_t PID;
	uint64_t deadline;
	struct PIT_deadline* next;
	struct PIT_deadline* prev;
};

void apic_initpit(void);

/* delay in milliseconds, but not accurate until 10ms */
void apic_addDeadline(pid_t PID, uint64_t delay);

uint64_t apic_pit_time(void);

void apic_PIThandler(uint64_t internal);

#endif /* APIC_PIT_H */
