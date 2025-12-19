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

#include <kernel/core/kentry.h>
#include <kernel/core/acpitables.h>
#include <kernel/core/mm_init.h>
#include <kernel/core/gdt.h>

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

struct mb2_info_t {
	const uint32_t total_size;
	const uint32_t reserved;
} __attribute__((packed)) header;

extern volatile struct gdt_t gdt[GDT_NUM_ENTRIES];

static volatile struct mb2_tag_t* memmap_tag;

static void first_segment(uint64_t* handle) {
	*handle = 0;
}

static void next_segment(uint64_t* handle, struct mem_segment_t* seg) {
	uint64_t addr = (uint64_t)&memmap_tag->tag.memmap.entries + *handle * memmap_tag->tag.memmap.entry_size;
	if (addr >= (uint64_t)memmap_tag + memmap_tag->size) {
		seg->base = 0;
		seg->size = 0;
		return;
	}

	struct mb2_tag_memmap_entry_t* entry = (struct mb2_tag_memmap_entry_t*)addr;
	seg->base = entry->base;
	seg->size = entry->length;

	switch (entry->type) {
		case MB2_MEMTYPE_AVLB:
			seg->type = MEM_AVL;
			break;
		case MB2_MEMTYPE_ACPI:
		case MB2_MEMTYPE_PRES:
			seg->type = MEM_PRS;
			break;
		default:
			seg->type = MEM_CLM;
			break;
	}

	(*handle)++;
}


void multiboot2_init(struct mb2_info_t* info) {
	// parse bootInformation
	uint64_t i = sizeof(*info);
	const uint64_t maxi = info->total_size;

	while (i < maxi) {
		struct mb2_tag_t* tag = (struct mb2_tag_t*)(((uint8_t*)info) + i);
		switch (tag->type) {
			case MBITAG_TYPE_MEMMAP:
				memmap_tag = tag;

				mm_init(first_segment, next_segment);
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

	boot_context.gdt = &gdt;
	return;
}
