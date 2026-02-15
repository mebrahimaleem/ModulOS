/* mergesort.h - mergesort implementation */
/* Copyright (C) 2025-2026  Ebrahim Aleem
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

#include <lib/mergesort.h>

void* mergesort_ll_inplace_ul(void* ll, void** (*next)(void*), unsigned long (*value)(void*)) {
    if (!ll || !*next(ll)) {
        return ll;
    }

    void *mid, *right, *left;
    void *fast;
    void *head, *tail;

    // find mid
    mid = ll;
    fast = *next(ll);

    while (fast && *next(fast)) {
        mid = *next(mid);
        fast = *next(*next(fast));
    }

    right = *next(mid);
    *next(mid) = 0;

    left  = mergesort_ll_inplace_ul(ll, next, value);
    right = mergesort_ll_inplace_ul(right, next, value);

    // merge
    if (!left) return right;
    if (!right) return left;

    if (value(left) <= value(right)) {
        head = left;
        left = *next(left);
    } else {
        head = right;
        right = *next(right);
    }

    tail = head;

    while (left && right) {
        if (value(left) <= value(right)) {
            *next(tail) = left;
            left = *next(left);
        } else {
            *next(tail) = right;
            right = *next(right);
        }
        tail = *next(tail);
    }

    *next(tail) = left ? left : right;

    return head;
}
