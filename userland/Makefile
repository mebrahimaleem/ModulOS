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

SYSROOT := $(abspath $(OBJ_DIR)/../userland_files/)

U_CFLAGS := -fno-pie -static \
						-ffreestanding -nostdlibinc -nostdlib \
						--sysroot=$(SYSROOT)/ -isystem $(SYSROOT)/usr/include/ \
						-Wall \
						$(CDEBUG)

ifndef DEBUG
U_CFLAGS := $(U_CFLAGS)
endif

LIB := $(SYSROOT)/usr/lib

.PHONY: all
all: build

%/:
	mkdir -p $@

.PHONY: build
build: libc shell

mlibc/:
	git clone https://github.com/managarm/mlibc.git $@
	git -C $@ checkout v6.3.1
	git -C $@ apply ../mlibc-patch/mlibc-v6-3-1-port.patch
	cp -r mlibc-patch/* mlibc/

$(OBJ_DIR)/mlibc-meson-setup: | mlibc/ $(OBJ_DIR)/libc-build/ $(SYSROOT)/usr/
	meson setup \
		--native-file mlibc/ci/linux-x86_64-clang.cross-file \
		--cross-file mlibc/ci/modulos.cross-file \
		--prefix $(SYSROOT)/usr/ \
		--reconfigure \
		-Ddefault_library=static \
		$(OBJ_DIR)/libc-build \
		mlibc/
	touch $@

.PHONY: libc
libc: $(OBJ_DIR)/mlibc-meson-setup
	cp -u -r mlibc-patch/* mlibc/
	ninja -C $(OBJ_DIR)/libc-build/
	meson install --only-changed -C $(OBJ_DIR)/libc-build/

$(LIB)/crt1.o: | libc
$(LIB)/libc.a: | libc

dash/:
	git clone https://git.kernel.org/pub/scm/utils/dash/dash.git
	git -C $@ checkout v0.5.13.3

$(OBJ_DIR)/dash-setup: $(LIB)/crt1.o $(LIB)/libc.a | dash/ $(OBJ_DIR)/dash-build/ $(SYSROOT)/usr/
	make -C $(OBJ_DIR)/dash-build/ distclean || true
	env -C dash/ ./autogen.sh
	env -C $(OBJ_DIR)/dash-build/ \
		CC=clang LD=ld.lld \
		AR=llvm-ar NM=llvm-nm RANLIB=llvm-ranlib OBJCOPY=llvm-objcopy OBJDUMP=llvm-objdump STRIP=llvm-strip AS=llvm-as \
		CFLAGS="$(U_CFLAGS)" \
		LDFLAGS="$(U_CLFAGS) -fuse-ld=lld -L$(LIB)/" \
		LIBS="$(LIB)/crt1.o $(LIB)/libc.a" \
		$(CURDIR)/dash/configure \
		--prefix $(SYSROOT)/usr/ \
		--host=x86_64-unkown-modulos-elf
	touch $@
 
.PHONY: shell
shell: $(OBJ_DIR)/dash-setup | $(SYSROOT)/bin/
	make -C $(OBJ_DIR)/dash-build/ install
	ln -f -r -s $(SYSROOT)/usr/bin/dash $(SYSROOT)/bin/sh
