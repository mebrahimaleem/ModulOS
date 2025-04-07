/* acpica.h - acpica exposed functions */
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

#ifndef ACPI_ACPICA_H
#define ACPI_ACPICA_H

#ifndef ACPICA_EXPOSE
#define ACPICA_EXPOSE

uint8_t acpiinit(void);

void* acpi_getMadt(void);

#endif /* ACPICA_EXPOSE */

#endif /* ACPI_ACPICA_H */
