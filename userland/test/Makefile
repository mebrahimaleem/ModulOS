# Makefile - userland test executable build toolchain
# Copyright (C) 2026  Ebrahim Aleem
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

USERLAND_DIR := $(OBJ_DIR)/../userland_files/
OBJ_DIR := $(OBJ_DIR)/test/

LCL_TARGETS := \
								$(USERLAND_DIR)/hello_world.txt \
								$(USERLAND_DIR)/test

include $(SRC_TREE_ROOT)/scripts/Makefile.kcflags

%/:
	-mkdir -p $@

.PHONY: all
all: build

.PHONY: build
build: $(LCL_TARGETS)

$(USERLAND_DIR)/hello_world.txt: | $(USERLAND_DIR)
	echo Hello, World! > $@

$(USERLAND_DIR)/test: $(OBJ_DIR)/test.o | $(USERLAND_DIR)
	$(CC) -g -o $@ $(LD_USR_FLAGS) -fuse-ld=lld $^

$(OBJ_DIR)/test.o: test.S | $(OBJ_DIR)
	$(CC) -g $(LD_USR_FLAGS) -Ilibc/ -c -o $@ $<

