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


# Debug options

DEBUG := 1

# Boot options

BUILD_BOOT_MULTIBOOT2 := 1

# Optional core modules

BUILD_KERNEL_GRAPHICSBASE := 1

# Optional driver modules


# End of options

SRC_TREE_ROOT := .
OBJ_DIR := build

KERNEL_TOOLCHAIN_PREFIX := x86_64-elf-

export

KERNEL_CC := $(KERNEL_TOOLCHAIN_PREFIX)gcc
KERNEL_LD := $(KERNEL_TOOLCHAIN_PREFIX)ld
KERNEL_STRIP := $(KERNEL_TOOLCHAIN_PREFIX)strip

SUBDIRS := kernel boot drivers
SUBDIR_TARGETS := $(OBJ_DIR)/kernel.a $(OBJ_DIR)/boot.a $(OBJ_DIR)/bootstub.img

COPY_DOC_TO := $(OBJ_DIR)/rootfs/doc/ModulOS/
COPY_DOC := $(COPY_DOC_TO)COPYING

COPY_BOOT_TO := $(OBJ_DIR)/rootfs/boot/
COPY_BOOT := $(COPY_BOOT_TO)modulos

TEST_RUNTIME_DIR := $(OBJ_DIR)/test

SRC := $(shell find . -type f \( -name "*.c" -o -name "*.S" -o -name "*.h" \))

KERNEL_LIB_DIR := $(dir $(shell $(KERNEL_CC) -mno-red-zone -print-libgcc-file-name))

.PHONY: build
build: $(OBJ_DIR)/modulos.img

.PHONY: all
all: index build

.PHONY: index
index:
	ctags --C-kinds=+pxzL -R $(SRC)
	echo $(SRC) > cscope.files
	cscope -q -R -b -i cscope.files

.PHONY: clean
clean:
	-rm -rd $(OBJ_DIR)/ tags cscope.*

.PHONY: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ build

%/:
	-mkdir -p $@

$(SUBDIR_TARGETS): %: $(SUBDIRS)

$(COPY_DOC): $(COPY_DOC_TO)%: % | $(COPY_DOC_TO)
	cp $< $|

$(COPY_BOOT): $(COPY_BOOT_TO)%: $(OBJ_DIR)/% | $(COPY_BOOT_TO)
	cp $< $|

$(OBJ_DIR)/fs.img: $(COPY_DOC) $(COPY_BOOT)
	truncate -s 4040M $@
	yes | mke2fs -L rootfs -d $(OBJ_DIR)/rootfs/ -t ext4 $@

$(OBJ_DIR)/modulos.img: $(OBJ_DIR)/bootstub.img $(OBJ_DIR)/fs.img
	cp $< $@
	dd if=$(OBJ_DIR)/fs.img of=$@ seek=13 count=1010 bs=4M conv=notrunc

$(OBJ_DIR)/modulos.ld: $(SUBDIRS)
	cat $$(find $(OBJ_DIR)/lds -type l -name "*.ld" | awk -F/ '{print $$NF, $$0}' | sort \
		| cut -d' ' -f2-) > $@

$(OBJ_DIR)/modulos: $(OBJ_DIR)/modulos-dbg
	$(KERNEL_STRIP) -s -o $@ $<

$(OBJ_DIR)/modulos-dbg: $(OBJ_DIR)/modulos.ld $(OBJ_DIR)/boot.a $(OBJ_DIR)/kernel.a
	$(KERNEL_LD) -o $@ -T $^ -L $(KERNEL_LIB_DIR) -lgcc
