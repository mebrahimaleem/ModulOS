# Makefile - build toolchain
# Copyright (C) 2025  Ebrahim Aleem
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>
#

KERNEL_CC := x86_64-elf-gcc
KERNEL_LD := x86_64-elf-ld
KERNEL_STRIP := x86_64-elf-strip

obj := $(CURDIR)/build
incl := $(CURDIR)/include

KERNEL_REQS_S := $(shell find multiboot2/ -type f -name "*.S")
KERNEL_REQS_C := $(shell find multiboot2/ -type f -name "*.c")

KERNEL_TARGETS_S := $(patsubst %.S,$(obj)/%.o,$(KERNEL_REQS_S))
KERNEL_TARGETS_C := $(patsubst %.c,$(obj)/%.o,$(KERNEL_REQS_C))
KERNEL_TARGETS := $(KERNEL_TARGETS_S) $(KERNEL_TARGETS_C)

ALL_REQS_S := $(KERNEL_REQS_S)
ALL_REQS_C := $(KERNEL_REQS_C)
ALL_REQS := $(ALL_REQS_S) $(ALL_REQS_C)

ALL_TARGETS_S := $(KERNEL_TARGETS_S)
ALL_TARGETS_C := $(KERNEL_TARGETS_C)
ALL_TARGETS := $(ALL_TARGETS_S) $(ALL_TARGETS_C)

CWARN := -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls \
	-Wnested-externs -Winline -Wno-long-long -Wconversion -Wstrict-prototypes
CFLAGS := $(CWARN) -O2 -static -mcmodel=kernel -ffreestanding -fomit-frame-pointer -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -c -g -F dwarf -I $(CURDIR)/include/

export

.PHONY: all
all: build

.PHONY: build
build: $(obj)/modulos.iso

.PHONY: clean
clean:
	-rm -rd build/*

%/:
	-mkdir -p $@

$(obj)/modulos.iso: cfg/grub.cfg $(obj)/modulos LICENSE | $(obj)/iso/boot/grub/
	cp $(obj)/modulos $(obj)/iso/boot/
	cp $< $(obj)/iso/boot/grub/
	cp LICENSE $(obj)/iso/
	grub-mkrescue -o $@ $(obj)/iso/

$(obj)/modulos: cfg/kernel.ld $(KERNEL_TARGETS) | $(obj)/
	$(KERNEL_LD) -o $@-dbg -T $^
	$(KERNEL_STRIP) -s -o $@ $@-dbg

# Multiboot2
$(ALL_TARGETS_S): $(obj)/multiboot2/%.o: multiboot2/%.S | $(obj)/multiboot2/
	$(MAKE) -C multiboot2/ $@

$(ALL_TARGETS_C): $(obj)/multiboot2/%.o: multiboot2/%.c $(incl)/multiboot2/%.h | $(obj)/multiboot2/
	$(MAKE) -C multiboot2/ $@
