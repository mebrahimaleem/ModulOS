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
export DEBUG_LOGGING = 1

# Global options

# Optional boot modules

export BUILD_BOOT_MULTIBOOT2 = 1


# Optional core modules

export BUILD_KERNEL_GRAPHICSBASE = 1

# Optional driver modules

export BUILD_DRIVERS_SERIAL = 1

# End of options

export SRC_TREE_ROOT = .
export OBJ_DIR = build

export CC := clang
export AR := llvm-ar
export STRIP := llvm-strip

SUBDIRS := kernel boot drivers test
KERNEL_TARGETS := \
									$(OBJ_DIR)/kernel.a
BOOT_TARGETS := \
									$(OBJ_DIR)/boot.a \
									$(OBJ_DIR)/esp.img
DRIVERS_TARGETS := \
									$(OBJ_DIR)/drivers.a
TEST_TARGETS := \
									$(OBJ_DIR)/test_lib.a \
									$(OBJ_DIR)/test_testsuite.a

TEST_CANIDATES := $(patsubst $(OBJ_DIR)/test_%.a,test-%,$(TEST_TARGETS))
TEST_EXEC := $(basename $(TEST_TARGETS))

COPY_DOC_TO := $(OBJ_DIR)/rootfs/doc/ModulOS/
COPY_DOC := $(COPY_DOC_TO)COPYING

SRC := $(shell find . -type f \( -name "*.c" -o -name "*.S" -o -name "*.h" \))

include $(SRC_TREE_ROOT)/scripts/Makefile.kcflags

.PHONY: build
build: $(OBJ_DIR)/modulos.img

.PHONY: all
all: index build test-all

.PHONY: index
index:
	ctags --C-kinds=+pxzL -R $(SRC)
	echo $(SRC) > cscope.files
	cscope -q -R -b -i cscope.files

.PHONY: clean
clean:
	-rm -rd $(OBJ_DIR)/ tags cscope.*

.PHONY: test-all
test-all: $(TEST_CANIDATES)

.PHONY: $(TEST_CANIDATES)
$(TEST_CANIDATES): test-%: $(OBJ_DIR)/test_%
	./$<

.PHONY: $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ build

%/:
	-mkdir -p $@

$(KERNEL_TARGETS): kernel
$(BOOT_TARGETS): boot
$(DRIVERS_TARGETS): drivers

$(TEST_TARGETS): test

$(TEST_EXEC): %: %.a $(OBJ_DIR)/boot.a $(OBJ_DIR)/kernel.a $(OBJ_DIR)/drivers.a
	$(CC) -O0 -g -std=c23 $(CWARN) -fuse-ld=lld -o $@ $^

$(COPY_DOC): $(COPY_DOC_TO)%: % | $(COPY_DOC_TO)
	cp $< $|

$(OBJ_DIR)/stub.img: | $(OBJ_DIR)/
	truncate -s 4G $@
	parted -s $@ mklabel gpt
	parted -s $@ mkpart ESP fat32 4MiB 52MiB
	parted -s $@ set 1 esp on
	parted -s $@ mkpart rootfs ext4 52MiB 100%

$(OBJ_DIR)/stub-esp.dummy: $(OBJ_DIR)/stub.img $(OBJ_DIR)/modulos $(OBJ_DIR)/esp.img
	mcopy -o -i $(OBJ_DIR)/esp.img $(OBJ_DIR)/modulos ::/
	dd if=$(OBJ_DIR)/esp.img of=$< seek=1 count=12 bs=4M conv=notrunc
	touch $@

# 54525952 is for 52MiB offset (512 * 1024 * 1024)
# 1034240 is for 52MiB initial (4MiB align + 48MiB ESP) and 4MiB tail for gpt
$(OBJ_DIR)/stub-fs.dummy: $(OBJ_DIR)/stub.img $(COPY_DOC)
	yes | mke2fs -L rootfs -E offset=54525952 -d $(OBJ_DIR)/rootfs/ -t ext4 \
		-b 4096 $< 1034240
	touch $@

$(OBJ_DIR)/modulos.img: $(OBJ_DIR)/stub.img $(OBJ_DIR)/stub-esp.dummy $(OBJ_DIR)/stub-fs.dummy
	@echo -n "\033[0;32m"
	ln -f -s -r $< $@
	@echo -n "\033[0m"

$(OBJ_DIR)/modulos.ld: $(filter-out test,$(SUBDIRS))
	cat $$(find $(OBJ_DIR)/lds -type l -name "*.ld" | awk -F/ '{print $$NF, $$0}' | sort \
		| cut -d' ' -f2-) > $@

$(OBJ_DIR)/modulos: $(OBJ_DIR)/modulos-dbg
	$(STRIP) -s -o $@ $<

$(OBJ_DIR)/modulos-dbg: $(OBJ_DIR)/modulos.ld $(OBJ_DIR)/boot.a $(OBJ_DIR)/kernel.a $(OBJ_DIR)/drivers.a
	$(CC) -o $@ $(LTO) $(LD_FLAGS) -fuse-ld=lld -T $^
