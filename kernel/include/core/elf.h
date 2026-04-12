/* elf.h - Executable and Linking Format loading interface */
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

#ifndef KERNEL_CORE_ELF_H
#define KERNEL_CORE_ELF_H

#include <stdint.h>

#include <kernel/core/fs.h>
#include <kernel/core/process.h>

extern uint8_t elf_is_elf(struct fs_handle_t* file);

extern struct pcb_t* elf_load(struct fs_handle_t* file, uint64_t pid);

#endif /* KERNEL_CORE_ELF_H */
