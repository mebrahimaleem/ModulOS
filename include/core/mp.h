/* mp.h - multiple processor management */
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

#ifndef CORE_MP_H
#define CORE_MP_H

extern uint64_t mp_bootstrap;
extern uint64_t mp_pdpt0;

extern uint8_t mp_loading;
extern uint64_t mp_rsp;
extern struct {
	uint8_t len;
	uint64_t off;
} __attribute__((packed)) mp_gdtptr;

void mp_initall(void);

#endif /* CORE_MP_H */

