/* framebuffer.c - framebuffer implementaion */
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

#include <stdint.h>

#include <graphicsbase/framebuffer.h>

#define BYTES_PER_PIXEL	4

static uint32_t convert_color(struct framebuffer_t* framebuffer, uint8_t r, uint8_t g, uint8_t b) {
	switch (framebuffer->mode) {
		case VIDEO_RGB_XRGB8888:
			return ((uint32_t)r << 16) + ((uint32_t)g << 8) + (uint32_t)b;
		default:
			return 0;
	}
}

void framebuffer_fillrect(
		struct framebuffer_t* framebuffer, uint32_t startx, uint32_t starty, uint32_t width,
		uint32_t height, uint8_t r, uint8_t g, uint8_t b) {

	uint8_t* row =
		(uint8_t*)(framebuffer->addr + starty * framebuffer->pitch + startx * BYTES_PER_PIXEL);
	volatile uint32_t* loc;
	uint32_t color = convert_color(framebuffer, r, g, b);

	uint32_t y, x;

	for (y = 0; y < height; y++) {
		loc = (volatile uint32_t*)row;
		for (x = 0; x < width; x++) {
			*(loc++) = color;
		}
		row += framebuffer->pitch;
	}
}
