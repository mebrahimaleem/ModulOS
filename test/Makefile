# Makefile - test suite build makefile
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

OBJ_DIR := $(OBJ_DIR)/test

SRC := $(shell find . -type f -name "*.c")
OBJ := $(patsubst %.c,$(OBJ_DIR)/%.o,$(SRC))

include $(SRC_TREE_ROOT)/scripts/Makefile.kcflags

OBJ_LIB := $(filter $(OBJ_DIR)/./lib/%,$(OBJ))

TARGETS :=

define add_test
$(OBJ_DIR)/test_$(1).a: $(filter $(OBJ_DIR)/./$(1)/%,$(OBJ))
	$(AR) rcs $$@ $$^

$(OBJ_DIR)/../test_$(1).a: $(OBJ_DIR)/test_helpers.a $(OBJ_DIR)/test_$(1).a
	echo "CREATE $$@ \n\
				ADDLIB $$< \n\
				ADDLIB $(OBJ_DIR)/test_$(1).a \n\
				SAVE \n\
				END" | $(AR) -M
$(eval TARGETS := $(TARGETS) $(OBJ_DIR)/../test_$(1).a)
endef

$(eval $(call add_test,lib))
$(eval $(call add_test,testsuite))

.PHONY: all
all: build

.PHONY: build
build: $(TARGETS)

$(OBJ_DIR)/test_helpers.a: $(filter $(OBJ_DIR)/./helpers/%,$(OBJ))
	$(AR) rcs $@ $^

$(OBJ): $(OBJ_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) -std=c23 -O0 -g $(CWARN) -I $(SRC_TREE_ROOT)/include/ -I include/ -c -o $@ $<
