/* syscall.h - ModulOS system call interface */
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

extern unsigned long int _syscall(unsigned long int v, unsigned long int argc, unsigned long int* argv);

static inline unsigned long int _syscall_0(
																						unsigned long int v) {
	return _syscall(v, 0, 0);
}

static inline unsigned long int _syscall_1(
																						unsigned long int v,
																						unsigned long int arg0) {
	unsigned long payload[1] = { arg0 };
	return _syscall(v, 1, payload);
}

static inline unsigned long int _syscall_2(
																						unsigned long int v,
																						unsigned long int arg0,
																						unsigned long int arg1) {
	unsigned long payload[2] = { arg0, arg1 };
	return _syscall(v, 2, payload);
}

static inline unsigned long int _syscall_3(
																						unsigned long int v,
																						unsigned long int arg0,
																						unsigned long int arg1,
																						unsigned long int arg2) {
	unsigned long payload[3] = { arg0, arg1, arg2 };
	return _syscall(v, 3, payload);
}
