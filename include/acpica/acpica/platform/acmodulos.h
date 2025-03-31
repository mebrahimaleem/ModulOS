/* acmodulos.h - ModulOS specific defines for ACPICA */
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

#ifndef __ACMODULOS_H__
#define __ACMODULOS_H__

#define ACPI_MACHINE_WIDTH 64

#define ACPI_DEBUGGER

#ifdef DEBUG
#define ACPI_DEBUG_OUTPUT 1
#endif

#define ACPI_USE_LOCAL_CACHE

#define ACPI_MUTEX_TYPE ACPI_OSL_MUTEX

#define ACPI_USE_DO_WHILE_0 do a while(0)

#include <stdint.h>
#define COMPILER_DEPENDENT_INT64 int64_t
#define COMPILER_DEPENDENT_UINT64 uint64_t

#ifndef ACPI_INLINE
#define ACPI_INLINE static inline
#endif

#define ACPICA_USE_NATIVE_DIVIDE

#define ACPI_DIV_64_BY_32(n, n_hi, n_lo, d32, q32, r32) \
{ \
 q32 = n / d32; \
 r32 = n % d32; \
}

#define ACPI_SHIFT_RIGHT_64(n, n_hi, n_lo) \
{ \
 n <<= 1; \
}

#ifndef ACPI_UNUSED_VAR
#define ACPI_UNUSED_VAR
#endif

#include <core/atomic.h>
#define ACPI_SPINLOCK spinlock_t
#define ACPI_SEMAPHORE semaphore_t
#define ACPI_MUTEX mutex_t

#include <core/cpulowlevel.h>
#define ACPI_FLUSH_CPU_CACHE() wbinvd()

#include <acpica/mutexsync.h>
#define ACPI_ACQUIRE_GLOBAL_LOCK(facsPtr, acc) acc = acpi_acquire_global_lock(facsPtr)
#define ACPI_RELEASE_GLOBAL_LOCK(facsPtr, pen) pen = acpi_release_global_lock(facsPtr)

#endif /* __ACMODULOS_H__ */
