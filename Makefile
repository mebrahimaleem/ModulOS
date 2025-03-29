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
#

KERNEL_CC := x86_64-elf-gcc
KERNEL_LD := x86_64-elf-ld
KERNEL_STRIP := x86_64-elf-strip

obj := $(CURDIR)/build
incl := $(CURDIR)/include
test := $(CURDIR)/test

KERNEL_REQS_S := $(shell find multiboot2/ -type f -name "*.S") $(shell find core/ -type f -name "*.S")
KERNEL_REQS_C := $(shell find multiboot2/ -type f -name "*.c") $(shell find core/ -type f -name "*.c") $(shell find acpica/ -type f -name "*.c")

KERNEL_TARGETS_S := $(patsubst %.S,$(obj)/%.o,$(KERNEL_REQS_S))
KERNEL_TARGETS_C := $(patsubst %.c,$(obj)/%.o,$(KERNEL_REQS_C))
KERNEL_TARGETS := $(KERNEL_TARGETS_S) $(KERNEL_TARGETS_C)

ALL_REQS_S := $(KERNEL_REQS_S)
ALL_REQS_C := $(KERNEL_REQS_C)
ALL_REQS := $(ALL_REQS_S) $(ALL_REQS_C)

ALL_TARGETS_S := $(KERNEL_TARGETS_S)
ALL_TARGETS_C := $(KERNEL_TARGETS_C)
ALL_TARGETS := $(ALL_TARGETS_S) $(ALL_TARGETS_C)

MULTIBOOT2_TARGETS_S := $(filter $(obj)/multiboot2/%,$(ALL_TARGETS_S))
MULTIBOOT2_TARGETS_C := $(filter $(obj)/multiboot2/%,$(ALL_TARGETS_C))
MULTIBOOT2_TARGETS := $(MULTIBOOT2_TARGETS_S) $(MULTIBOOT2_TARGETS_C)

CORE_TARGETS_S := $(filter $(obj)/core/%,$(ALL_TARGETS_S))
CORE_TARGETS_C := $(filter $(obj)/core/%,$(ALL_TARGETS_C))
CORE_TARGETS := $(CORE_TARGETS_S) $(CORE_TARGETS_C)

ACPICA_TARGETS_C := $(filter $(obj)/acpica/%,$(ALL_TARGETS_C))
ACPICA_TARGETS := $(ACPICA_TARGETS_C)

CWARN := -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls \
	-Wnested-externs -Winline -Wno-long-long -Wconversion -Wstrict-prototypes
CDEBUG := -D DEBUG -O0
CFLAGS := $(CWARN) -O2 $(CDEBUG) -D_MODULOS -static -fno-pie -mcmodel=kernel -ffreestanding -fomit-frame-pointer -fno-asynchronous-unwind-tables \
	-mno-red-zone -mno-mmx -mno-sse -mno-sse2 -c -g -F dwarf -I $(CURDIR)/include/

export

.PHONY: all
all: build

.PHONY: build
build: $(obj)/modulos.img

.PHONY: clean
clean:
	-rm -rd build/* test/*

.PHONY: simulateqemu
simulateqemu: $(obj)/modulos.img | $(test)/
	qemu-system-x86_64 -s -S -smp sockets=1,cores=2,threads=1 -serial vc -serial file:$(test)/serial -d int -m 16G -monitor stdio -drive format=raw,file=$<,if=ide,media=disk

.PHONY: debuggdb
debuggdb: $(obj)/modulos-dbg
	-gdb -q -tui \
		--init-eval-command="tar rem localhost:1234" \
		--init-eval-command="la sp" \
		--init-eval-command="sy $<" \
		--init-eval-command="c"


.PHONY: rootfs
rootfs: | $(obj)/iso/
	mkdir -p \
		$|bin/ \
		$|boot/ \
		$|dev/ \
		$|etc/ \
		$|home/ \
		$|lib/ \
		$|media/ \
		$|mnt/ \
		$|opt/ \
		$|proc/ \
		$|root/ \
		$|sbin/ \
		$|srv/ \
		$|sys/ \
		$|tmp/ \
		$|usr/ $|usr/include/ $|usr/lib/ $|usr/libexec/ $|usr/local/ $|usr/share/ \
		$|var/ $|var/log/ $|var/mail/ $|var/spool/ $|var/src/ $|var/tmp/

%/:
	-mkdir -p $@

$(obj)/modulos.img: $(obj)/modulos cfg/grub.cfg LICENSE | rootfs
	mkdir -p $(obj)/iso/boot/grub/ $(obj)/iso/usr/share/doc/ModulOS/
	cp $< $(obj)/iso/boot/
	cp cfg/grub.cfg $(obj)/iso/boot/grub/
	cp LICENSE $(obj)/iso/usr/share/doc/ModulOS/

	grub-mkrescue -o $@ $(obj)/iso/

$(obj)/modulos: $(obj)/modulos-dbg | $(obj)/
	$(KERNEL_STRIP) -s -o $@ $<

$(obj)/modulos-dbg: cfg/kernel.ld $(KERNEL_TARGETS) | $(obj)/
	$(KERNEL_LD) -o $@ -T $^

# Multiboot2
$(MULTIBOOT2_TARGETS_S): $(obj)/multiboot2/%.o: multiboot2/%.S | $(obj)/multiboot2/
	$(MAKE) -C multiboot2/ $@

$(MULTIBOOT2_TARGETS_C): $(obj)/multiboot2/%.o: multiboot2/%.c $(incl)/multiboot2/%.h | $(obj)/multiboot2/
	$(MAKE) -C multiboot2/ $@

# Core
$(CORE_TARGETS_S): $(obj)/core/%.o: core/%.S | $(obj)/core/
	$(MAKE) -C core/ $@

$(CORE_TARGETS_C): $(obj)/core/%.o: core/%.c $(incl)/core/%.h | $(obj)/core/
	$(MAKE) -C core/ $@

# Acpica
$(ACPICA_TARGETS_C): $(obj)/acpica/%.o: acpica/%.c | $(obj)/acpica/
	$(MAKE) -C acpica/ $@
