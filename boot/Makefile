# Makefile - boot build makefile
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

SRC_TREE_ROOT := ../$(SRC_TREE_ROOT)
include $(SRC_TREE_ROOT)/scripts/Makefile.header

CC := $(KERNEL_TOOLCHAIN_PREFIX)gcc

OBJ_DIR := $(OBJ_DIR)/boot

ifdef BUILD_BOOT_MULTIBOOT2
$(call add_directory,multiboot2,MULTIBOOT2)
endif

ifdef BUILD_BOOT_GRAPHICSBASE
DEFINES := $(DEFINES) -DGRAPHICSBASE
endif

ifneq ($(words $(SRC_DIRS)), 1)
$(error Exactly a single bootloader can be built)
endif

include $(SRC_TREE_ROOT)/scripts/Makefile.kcflags
include $(SRC_TREE_ROOT)/scripts/Makefile.targets

.PHONY: all
all: build

.PHONY: build
build: $(OBJ_DIR)/../boot.a $(LDS) $(OBJ_DIR)/../bootstub.img

$(OBJ_DIR)/../bootstub.img: $(OBJ_DIR)/gpt.img $(OBJ_DIR)/esp.img
	truncate -s 4G $@
	dd if=$(OBJ_DIR)/gpt.img of=$@ seek=0 count=1 bs=4M conv=notrunc
	dd if=$(OBJ_DIR)/gpt.img of=$@ skip=1023 seek=1023 count=1 bs=4M conv=notrunc

.PHONY: $(OBJ_DIR)/gpt.img
$(OBJ_DIR)/gpt.img:
	$(MAKE) -C $(SRC_DIRS) ../$@

.PHONY: $(OBJ_DIR)/esp.img
$(OBJ_DIR)/esp.img:
	$(MAKE) -C $(SRC_DIRS) ../$@

$(OBJ_DIR)/boot.a: $(TARGETS) | $(BUILD_DIRS)
	rm -f $@
	$(AR) rcs $@ $(TARGETS)
