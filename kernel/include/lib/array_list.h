/* array_list.h - array list interface */
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

#ifndef KERNEL_LIB_ARRAY_LIST_H
#define KERNEL_LIB_ARRAY_LIST_H

#include <stdint.h>
#include <stddef.h>

struct array_list_t;

extern struct array_list_t* array_list_alloc(size_t init_cap, size_t growth, void* null);

extern uint64_t array_list_push(struct array_list_t* list, void* value);

extern void* array_list_get(struct array_list_t* list, uint64_t index);

extern void* array_list_remove(struct array_list_t* list, uint64_t index);

extern void array_list_clear(struct array_list_t* list, void (*free_func)(void*));

extern void array_list_free(struct array_list_t* list, void (*free_func)(void*));

extern struct array_list_t* array_list_dup(struct array_list_t* list, void* (*dup_func)(void*));

extern uint64_t array_list_count(struct array_list_t* list);

#endif /* KERNEL_LIB_ARRAY_LIST_H */

