/* cpulowlevel.h - cpu low level */
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


#ifndef CORE_CPULOWLEVEL_H
#define CORE_CPULOWLEVEL_H

void wbinvd(void);

void cpuGetMSR(uint32_t msr, uint32_t* lo, uint32_t* hi);

void cpuSetMSR(uint32_t msr, uint32_t lo, uint32_t hi);

void cpuid(uint32_t ieax, uint32_t iecx, uint32_t* oeax, uint32_t* oebx, uint32_t* oecx, uint32_t* oedx);

#endif /* CORE_CPULOWLEVEL_H */


