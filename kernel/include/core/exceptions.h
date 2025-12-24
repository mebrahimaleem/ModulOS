/* exceptions.h - exceptions interrrupt service routine interface */
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

#ifndef KERNEL_CORE_EXCEPTIONS_H
#define KERNEL_CORE_EXCEPTIONS_H

extern void isr_00(void) __attribute__((noreturn));
extern void isr_01(void) __attribute__((noreturn));
extern void isr_02(void) __attribute__((noreturn));
extern void isr_03(void) __attribute__((noreturn));
extern void isr_04(void) __attribute__((noreturn));
extern void isr_05(void) __attribute__((noreturn));
extern void isr_06(void) __attribute__((noreturn));
extern void isr_07(void) __attribute__((noreturn));
extern void isr_08(void) __attribute__((noreturn));
extern void isr_09(void) __attribute__((noreturn));
extern void isr_0a(void) __attribute__((noreturn));
extern void isr_0b(void) __attribute__((noreturn));
extern void isr_0c(void) __attribute__((noreturn));
extern void isr_0d(void) __attribute__((noreturn));
extern void isr_0e(void) __attribute__((noreturn));
extern void isr_10(void) __attribute__((noreturn));
extern void isr_11(void) __attribute__((noreturn));
extern void isr_12(void) __attribute__((noreturn));
extern void isr_13(void) __attribute__((noreturn));
extern void isr_14(void) __attribute__((noreturn));
extern void isr_15(void) __attribute__((noreturn));
extern void isr_1c(void) __attribute__((noreturn));
extern void isr_1d(void) __attribute__((noreturn));
extern void isr_1e(void) __attribute__((noreturn));

#endif /* KERNEL_CORE_EXCEPTIONS_H */
