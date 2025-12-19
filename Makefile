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

#export DEBUG = 1

# Global options

export BUILD_GRAPHICSBASE = 1

# Optional boot modules

export BUILD_BOOT_MULTIBOOT2 = 1


# Optional core modules


# Optional driver modules


# Auto include modules

ifdef BUILD_GRAPHICSBASE
export BUILD_BOOT_GRAPHICSBASE = 1
endif

ifdef BUILD_GRAPHICSBASE
export BUILD_KERNEL_GRAPHICSBASE = 1
endif

# End of options

export SRC_TREE_ROOT = .
export OBJ_DIR = build

export CC := clang
export AR := llvm-ar

SUBDIRS := kernel boot drivers
SUBDIR_TARGETS := \
									$(OBJ_DIR)/kernel.a \
									$(OBJ_DIR)/boot.a \
									$(OBJ_DIR)/bootstub.img \
									$(OBJ_DIR)/esp.img \
									$(OBJ_DIR)/drivers.a

COPY_DOC_TO := $(OBJ_DIR)/rootfs/doc/ModulOS/
COPY_DOC := $(COPY_DOC_TO)COPYING

TEST_RUNTIME_DIR := $(OBJ_DIR)/test

SRC := $(shell find . -type f \( -name "*.c" -o -name "*.S" -o -name "*.h" \))

include $(SRC_TREE_ROOT)/scripts/Makefile.kcflags

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

$(OBJ_DIR)/fs.img: $(COPY_DOC)
	truncate -s 4040M $@
	yes | mke2fs -L rootfs -d $(OBJ_DIR)/rootfs/ -t ext4 $@

$(OBJ_DIR)/modulos.img: $(OBJ_DIR)/bootstub.img $(OBJ_DIR)/fs.img $(OBJ_DIR)/esp.img $(OBJ_DIR)/modulos
	cp $< $@
	mcopy -o -i $(OBJ_DIR)/esp.img $(OBJ_DIR)/modulos ::/
	dd if=$(OBJ_DIR)/esp.img of=$@ seek=1 count=12 bs=4M conv=notrunc
	dd if=$(OBJ_DIR)/fs.img of=$@ seek=13 count=1010 bs=4M conv=notrunc

$(OBJ_DIR)/modulos.ld: $(SUBDIRS)
	cat $$(find $(OBJ_DIR)/lds -type l -name "*.ld" | awk -F/ '{print $$NF, $$0}' | sort \
		| cut -d' ' -f2-) > $@

$(OBJ_DIR)/modulos: $(OBJ_DIR)/modulos-dbg
	llvm-strip -s -o $@ $<

$(OBJ_DIR)/modulos-dbg: $(OBJ_DIR)/modulos.ld $(OBJ_DIR)/boot.a $(OBJ_DIR)/kernel.a $(OBJ_DIR)/drivers.a
	$(CC) -o $@ $(LTO) $(LD_FLAGS) -fuse-ld=lld -T $^
