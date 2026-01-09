/* framebuffer.h - framebuffer interface */
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

#ifndef KERNEL_GRAPHICSBASE_FRAMEBUFFER_H
#define KERNEL_GRAPHICSBASE_FRAMEBUFFER_H

#include <stdint.h>

struct framebuffer_t {
	uint64_t addr;
	uint32_t height;
	uint32_t width;
	uint32_t pitch;
	enum {
		VIDEO_NONE,
		VIDEO_RGB_XRGB8888
	} mode;
};

extern void framebuffer_fillrect(
		struct framebuffer_t* framebuffer, uint32_t startx, uint32_t starty, uint32_t width,
		uint32_t height, uint8_t r, uint8_t g, uint8_t b);

#endif /* KERNEL_GRAPHICSBASE_FRAMEBUFFER_H */

