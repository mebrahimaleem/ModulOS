/* msr.h - Model specific register routine interface */
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

#ifndef CORE_MSR_H
#define CORE_MSR_H

#include <stdint.h>

#define MSR_APIC_BASE	0x0000001B

extern void msr_write(uint64_t msr, uint64_t val);

extern uint64_t msr_read(uint64_t msr);

#endif /* CORE_MSR_H */
