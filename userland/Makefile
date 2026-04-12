# Makefile - userland build toolchain
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

export SRC_TREE_ROOT := ../$(SRC_TREE_ROOT)
include $(SRC_TREE_ROOT)/scripts/Makefile.header

export OBJ_DIR := $(OBJ_DIR)/userland

SUBTARGETS := test

.PHONY: all
all: build

.PHONY: build
build: $(OBJ_DIR)/../userland_files/

$(OBJ_DIR)/../userland_files/: $(SUBTARGETS)

.PHONY: $(SUBTARGETS)
$(SUBTARGETS):
	$(MAKE) -C $@ build
