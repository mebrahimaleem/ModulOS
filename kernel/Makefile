# Makefile - kernel build makefile
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
AR := $(KERNEL_TOOLCHAIN_PREFIX)ar

OBJ_DIR := $(OBJ_DIR)/kernel

$(call add_directory,core,CORE)
$(call add_directory,lib,LIB)

ifdef BUILD_KERNEL_GRAPHICSBASE
$(call add_directory,graphicsbase,GRAPHICSBASE)
endif

include $(SRC_TREE_ROOT)/scripts/Makefile.kcflags
include $(SRC_TREE_ROOT)/scripts/Makefile.targets

.PHONY: all
all: build

.PHONY: build
build: $(OBJ_DIR)/../kernel.a $(LDS)

$(OBJ_DIR)/kernel.a: $(TARGETS) | $(BUILD_DIRS)
	rm -f $@
	$(AR) rcs $@ $(TARGETS)
