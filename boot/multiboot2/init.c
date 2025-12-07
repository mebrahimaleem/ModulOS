/* init.c - multiboot2 kernel init */
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

#include <multiboot2/init.h>

#include <core/kentry.h>
#include <core/earlymemory.h>
#include <core/acpitables.h>

#define MOD8_MASK	(uint64_t)0x7

#define MB2_MEMTYPE_AVLB	1
#define MB2_MEMTYPE_ACPI	3
#define MB2_MEMTYPE_PRES	4
#define MB2_MEMTYPE_DEFC	5

#define VIDEO_XRGB8888_REDPOS		0x10
#define VIDEO_XRGB8888_GREENPOS	0x08
#define VIDEO_XRGB8888_BLUEPOS		0x00
#define VIDEO_XRGB8888_REDMASK		0x08
#define VIDEO_XRGB8888_GREENMASK	0x08
#define VIDEO_XRGB8888_BLUEMASK	0x08
#define VIDEO_XRGB8888_BPP				32

struct mb2_tag_t {
	const uint32_t type;
	const uint32_t size;
} __attribute__((packed));

struct mb2_tag_memmap_entry_t {
	uint64_t base;
	uint64_t length;
	uint32_t type;
	uint8_t reserved[];
} __attribute__((packed));

struct mb2_tag_memmap_t {
	struct mb2_tag_t header;
	uint32_t entry_size;
	uint32_t entry_version;
} __attribute__((packed));

struct mb2_tag_rsdpv2_t {
	struct mb2_tag_t header;
	struct RSDP_t rsdp;
} __attribute__((packed));


#ifdef GRAPHICSBASE
struct mb2_tag_framebuffer_t {
	struct mb2_tag_t header;
	uint64_t framebuffer_addr;
	uint32_t framebuffer_pitch;
	uint32_t framebuffer_width;
	uint32_t framebuffer_height;
	uint8_t framebuffer_bpp;
	uint8_t framebuffer_type;
	uint16_t reserved; // extra byte of apdding to align union
	union {
		struct {
			uint8_t framebuffer_red_field_position;
			uint8_t framebuffer_red_mask_size;
			uint8_t framebuffer_green_field_position;
			uint8_t framebuffer_green_mask_size;
			uint8_t framebuffer_blue_field_position;
			uint8_t framebuffer_blue_mask_size;
		} __attribute__((packed)) rgb;
	} __attribute__((packed)) color_info;
} __attribute__((packed));
#endif /* GRAPHICSBASE */

struct mb2_info_t {
	const uint32_t total_size;
	const uint32_t reserved;
} __attribute__((packed)) header;

extern struct boot_context_t boot_context;

void multiboot2_init(volatile struct mb2_info_t* info) {
	memory_init_early();

	// parse bootInformation
	uint64_t i = sizeof(*info);
	const uint64_t maxi = info->total_size;

	while (i < maxi) {
		volatile struct mb2_tag_t* tag = (volatile struct mb2_tag_t*)(((uint8_t*)info) + i);
		switch (tag->type) {
			case MBITAG_TYPE_MEMMAP:
				volatile struct mb2_tag_memmap_t* memmap_tag = (volatile struct mb2_tag_memmap_t*)tag;
				uint64_t num_entries = (tag->size - sizeof(*memmap_tag)) / memmap_tag->entry_size;
				boot_context.num_memmap = num_entries;
				boot_context.memmap = kmalloc_early(sizeof(struct memmap_t) * num_entries);

				// copy all entries so that os can write over mbi
				uint32_t k = 0;
				for(uint32_t j = sizeof(*memmap_tag); j < tag->size; j += memmap_tag->entry_size) {
					volatile struct mb2_tag_memmap_entry_t* memmap_entry =
						(volatile struct mb2_tag_memmap_entry_t*)((uint8_t*)tag + j);
					boot_context.memmap[k++] = (struct memmap_t){
						.base = memmap_entry->base,
						.length = memmap_entry->length,
						.type = memmap_entry->type
					};
				}
				break;
			case MBITAG_TYPE_RSDPV2:
				boot_context.rsdp = ((volatile struct mb2_tag_rsdpv2_t*)tag)->rsdp;
				break;
#ifdef GRAPHICSBASE
			case MBITAG_TYPE_FRMBUF:
				volatile struct mb2_tag_framebuffer_t* framebuffer_tag =
					(volatile struct mb2_tag_framebuffer_t*)tag;

				// check video mode, and skip if not supported
				if (
					framebuffer_tag->color_info.rgb.framebuffer_red_field_position != VIDEO_XRGB8888_REDPOS ||
					framebuffer_tag->color_info.rgb.framebuffer_green_field_position != VIDEO_XRGB8888_GREENPOS ||
					framebuffer_tag->color_info.rgb.framebuffer_blue_field_position != VIDEO_XRGB8888_BLUEPOS ||
					framebuffer_tag->color_info.rgb.framebuffer_red_mask_size != VIDEO_XRGB8888_REDMASK ||
					framebuffer_tag->color_info.rgb.framebuffer_green_mask_size != VIDEO_XRGB8888_GREENMASK ||
					framebuffer_tag->color_info.rgb.framebuffer_blue_mask_size != VIDEO_XRGB8888_BLUEMASK ||
					framebuffer_tag->framebuffer_bpp != VIDEO_XRGB8888_BPP)
						break;

				boot_context.framebuffer = (struct framebuffer_t) {
					.addr = framebuffer_tag->framebuffer_addr,
					.height = framebuffer_tag->framebuffer_height,
					.width = framebuffer_tag->framebuffer_width,
					.pitch = framebuffer_tag->framebuffer_pitch,
					.mode = VIDEO_RGB_XRGB8888
				};
				break;
#endif /* GRAPHICSBASE */
			default:
				break;
		}

		i += tag->size;
		// enforce 8 byte alignment
		i = (i + 7) & ~MOD8_MASK;
	}

	return;
}
