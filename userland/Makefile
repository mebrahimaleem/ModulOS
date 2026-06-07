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
						$(CDEBUG) -UDEBUG

ifndef DEBUG
U_CFLAGS := $(U_CFLAGS)
endif

LIB := $(SYSROOT)/usr/lib

.PHONY: all
all: build

%/:
	mkdir -p $@

.PHONY: build
build: _libc _shell _utils _init

mlibc/:
	git clone https://github.com/managarm/mlibc.git $@
	git -C $@ checkout c367b780a47dc1d6c70d19a8379f4a74e5a7ed96
	git -C $@ apply ../mlibc-patch/mlibc-port.patch
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

.PHONY: _libc
_libc: $(OBJ_DIR)/mlibc-meson-setup
	cp -u -r mlibc-patch/* mlibc/
	ninja -C $(OBJ_DIR)/libc-build/
	meson install --only-changed -C $(OBJ_DIR)/libc-build/

$(LIB)/crt1.o: | _libc
$(LIB)/libc.a: | _libc

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
 
.PHONY: _shell
_shell: $(OBJ_DIR)/dash-setup | $(SYSROOT)/bin/
	make -C $(OBJ_DIR)/dash-build/ install
	ln -f -r -s $(SYSROOT)/usr/bin/dash $(SYSROOT)/bin/sh

sbase/:
	git clone git://git.suckless.org/sbase
	git -C $@ checkout c546c3a5724c81cee9a11d816a38ccdf17472129

$(OBJ_DIR)/utils-setup: $(LIB)/crt1.o $(LIB)/libc.a | sbase/ $(OBJ_DIR)/utils-build/ $(SYSROOT)/usr/share/man/
	make -C $(OBJ_DIR)/utils-build/ clean || true
	rsync -a --exclude='.git' sbase/ $(OBJ_DIR)/utils-build/
	sed -i \
			-e 's|^PREFIX = .*|PREFIX = $(SYSROOT)/usr/|' \
			-e 's|^#CC =.*|CC = clang|' \
			-e 's|^#AR =.*|AR = llvm-ar|' \
			-e 's|^RANLIB = .*|RANLIB = llvm-ranlib|' \
			-e '/^# tools/a \
	LD = ld.lld\nNM = llvm-nm\nOBJCOPY = llvm-objcopy\nOBJDUMP = llvm-objdump\nSTRIP = llvm-strip\nAS = llvm-as' \
			-e 's|^# CFLAGS .*|CFLAGS = $(U_CFLAGS)|' \
			-e 's|^# LDFLAGS .*|LDFLAGS = $(U_CFLAGS) -fuse-ld=lld -L$(LIB)/|' \
			$(OBJ_DIR)/utils-build/config.mk
	sed -i \
			-e 's|-o \$$@ \$$< \$$(LIB)|-o $$@ $$< $$(LIB) $(LIB)/crt1.o $(LIB)/libc.a|' \
			-e 's|-o \$$@ bc\.c \$$(LIB)|-o $$@ bc.c $$(LIB) $(LIB)/crt1.o $(LIB)/libc.a|' \
			-e 's|-o \$$@ \$$(MAKEOBJ) \$$(LIB)|-o $$@ $$(MAKEOBJ) $$(LIB) $(LIB)/crt1.o $(LIB)/libc.a|' \
			-e 's|-o \$$@ build/\*\.c \$$(LIB)|-o $$@ build/*.c $$(LIB) $(LIB)/crt1.o $(LIB)/libc.a|' \
			-e '/\$$(RANLIB) \$$@/a \
		$$(OBJCOPY) --weaken $$@' \
			$(OBJ_DIR)/utils-build/Makefile
	touch $@

.PHONY: _utils
_utils: $(OBJ_DIR)/utils-setup
	make -C $(OBJ_DIR)/utils-build/ install

$(OBJ_DIR)/init.o: init/init.c
	$(CC) $(U_CFLAGS) -c -o $@ $<

$(SYSROOT)/init: $(OBJ_DIR)/init.o $(LIB)/crt1.o $(LIB)/libc.a | $(SYSROOT)/
	$(CC) $(U_CFLAGS) -fuse-ld=lld -o $@ $^

.PHONY: _init
_init: $(SYSROOT)/init
