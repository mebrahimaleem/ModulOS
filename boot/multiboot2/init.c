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
#include <core/acpitables.h>
#include <core/mm.h>

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

struct mb2_tag_memmap_entry_t {
	uint64_t base;
	uint64_t length;
	uint32_t type;
	uint8_t reserved; // variable length
} __attribute__((packed));

struct mb2_tag_t {
	const uint32_t type;
	const uint32_t size;
	union {
		struct {
			uint32_t entry_size;
			uint32_t entry_version;
			struct mb2_tag_memmap_entry_t entries;
		} __attribute__((packed)) memmap;

		struct {
			struct RSDP_t rsdp;
		} __attribute__((packed)) rsdpv2;

#ifdef GRAPHICSBASE
		struct {
			uint64_t framebuffer_addr;
			uint32_t framebuffer_pitch;
			uint32_t framebuffer_width;
			uint32_t framebuffer_height;
			uint8_t framebuffer_bpp;
			uint8_t framebuffer_type;
			uint16_t reserved; // extra byte of padding to align union
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
		} __attribute__((packed)) framebuffer;
#endif /* GRAPHICSBASE */
	} tag;
} __attribute__((packed));

struct mem_segment_handle_t {
	uint64_t entry_size;
	uint64_t limit;
	uint64_t current;
	struct mb2_tag_memmap_entry_t* entries;
};

struct mb2_info_t {
	const uint32_t total_size;
	const uint32_t reserved;
} __attribute__((packed)) header;

extern struct boot_context_t boot_context;

static struct mem_segment_handle_t mem_handle;

static void next_segment(struct mem_segment_handle_t** handle) {
	do {
		(*handle)->current += (*handle)->entry_size;
		if ((*handle)->current >= (*handle)->limit) {
			*handle = 0;
			return;
		}
	} while (((struct mb2_tag_memmap_entry_t*)(*handle)->current)->type != MB2_MEMTYPE_AVLB);
	// TODO: handle other types
}

static struct mem_segment_handle_t* first_segment(void) {
	// always will be at least one entry
	mem_handle.current = (uint64_t)mem_handle.entries;

	// TODO: handle other types
	while (((struct mb2_tag_memmap_entry_t*)mem_handle.current)->type != MB2_MEMTYPE_AVLB) {
		mem_handle.current += mem_handle.entry_size;
		if (mem_handle.current >= mem_handle.limit) {
			return 0;
		}
	}
	return &mem_handle;
}

static uint64_t get_base(struct mem_segment_handle_t* handle) {
	return ((struct mb2_tag_memmap_entry_t*)handle->current)->base;
}

static size_t get_size(struct mem_segment_handle_t* handle) {
	return ((struct mb2_tag_memmap_entry_t*)handle->current)->length;
}

static void set_base(struct mem_segment_handle_t* handle, uint64_t base) {
	((struct mb2_tag_memmap_entry_t*)handle->current)->base = base;
}

static void set_size(struct mem_segment_handle_t* handle, size_t size) {
	((struct mb2_tag_memmap_entry_t*)handle->current)->length = size;
}

void multiboot2_init(struct mb2_info_t* info) {
	// parse bootInformation
	uint64_t i = sizeof(*info);
	const uint64_t maxi = info->total_size;

	while (i < maxi) {
		struct mb2_tag_t* tag = (struct mb2_tag_t*)(((uint8_t*)info) + i);
		switch (tag->type) {
			case MBITAG_TYPE_MEMMAP:
				mem_handle = (struct mem_segment_handle_t){
					.entry_size = tag->tag.memmap.entry_size,
					.limit = (uint64_t)tag + tag->size,
					.current = (uint64_t)&tag->tag.memmap.entries,
					.entries = &tag->tag.memmap.entries
				};

				mm_init(next_segment, first_segment, get_base, get_size, set_base, set_size);
				break;
			case MBITAG_TYPE_RSDPV2:
				boot_context.rsdp = tag->tag.rsdpv2.rsdp;
				break;
#ifdef GRAPHICSBASE
			case MBITAG_TYPE_FRMBUF:
				// check video mode, and skip if not supported
				if (
					tag->tag.framebuffer.color_info.rgb.framebuffer_red_field_position != VIDEO_XRGB8888_REDPOS ||
					tag->tag.framebuffer.color_info.rgb.framebuffer_green_field_position != VIDEO_XRGB8888_GREENPOS ||
					tag->tag.framebuffer.color_info.rgb.framebuffer_blue_field_position != VIDEO_XRGB8888_BLUEPOS ||
					tag->tag.framebuffer.color_info.rgb.framebuffer_red_mask_size != VIDEO_XRGB8888_REDMASK ||
					tag->tag.framebuffer.color_info.rgb.framebuffer_green_mask_size != VIDEO_XRGB8888_GREENMASK ||
					tag->tag.framebuffer.color_info.rgb.framebuffer_blue_mask_size != VIDEO_XRGB8888_BLUEMASK ||
					tag->tag.framebuffer.framebuffer_bpp != VIDEO_XRGB8888_BPP)
						break;

				boot_context.framebuffer = (struct framebuffer_t) {
					.addr = tag->tag.framebuffer.framebuffer_addr,
					.height = tag->tag.framebuffer.framebuffer_height,
					.width = tag->tag.framebuffer.framebuffer_width,
					.pitch = tag->tag.framebuffer.framebuffer_pitch,
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
