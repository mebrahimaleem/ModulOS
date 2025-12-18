/* isr.S - interrupt service routine entries interface */
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

#ifndef CORE_ISR_H
#define CORE_ISR_H

extern void isr_00(void);
extern void isr_01(void);
extern void isr_02(void);
extern void isr_03(void);
extern void isr_04(void);
extern void isr_05(void);
extern void isr_06(void);
extern void isr_07(void);
extern void isr_08(void);
extern void isr_09(void);
extern void isr_0a(void);
extern void isr_0b(void);
extern void isr_0c(void);
extern void isr_0d(void);
extern void isr_0e(void);
extern void isr_10(void);
extern void isr_11(void);
extern void isr_12(void);
extern void isr_13(void);
extern void isr_14(void);
extern void isr_15(void);
extern void isr_1c(void);
extern void isr_1d(void);
extern void isr_1e(void);

#endif /* CORE_ISR_H */
