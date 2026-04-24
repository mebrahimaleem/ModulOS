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

SRC_TREE_ROOT := ../$(SRC_TREE_ROOT)
include $(SRC_TREE_ROOT)/scripts/Makefile.header

OBJ_DIR := $(OBJ_DIR)/userland

U_CFLAGS := -g -fno-pie -static \
						-nostdlib -nodefaultlibs \
						-fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables \
						--sysroot=$(OBJ_DIR)/../userland_files/

LIB := $(OBJ_DIR)/../userland_files/usr/lib

.PHONY: all
all: build

.PHONY: build
build: libc $(OBJ_DIR)/../userland_files/bin/shell

%/:
	mkdir -p $@

.PHONY: libc
libc: $(OBJ_DIR)/mlibc-meson-setup | $(OBJ_DIR)/libc-build/
	ninja -C $|
	meson install --only-changed -C $|

$(OBJ_DIR)/mlibc-meson-setup: | $(OBJ_DIR)/libc-build/ $(OBJ_DIR)/../userland_files/usr/
	meson setup \
		--native-file mlibc/ci/linux-x86_64-clang.cross-file \
		--cross-file mlibc/ci/modulos.cross-file \
		--prefix $(abspath $(OBJ_DIR)/../userland_files/usr/) \
		--reconfigure \
		-Ddefault_library=static \
		$(OBJ_DIR)/libc-build \
		mlibc/
	touch $@

$(OBJ_DIR)/shell.o: shell/main.c | $(OBJ_DIR)
	$(CC) $(U_CFLAGS) -c -o $(OBJ_DIR)/shell.o $<

$(OBJ_DIR)/../userland_files/bin/shell: $(OBJ_DIR)/shell.o $(LIB)/crt1.o $(LIB)/libc.a | libc $(OBJ_DIR)/../userland_files/bin/
	$(CC) $(U_CFLAGS) -fuse-ld=lld -o $@ $(LIB)/crt1.o $< $(LIB)/libc.a
