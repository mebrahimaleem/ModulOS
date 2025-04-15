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
deps := $(obj)/deps

KERNEL_SRC_S := \
									$(shell find multiboot2/ -type f -name "*.S") \
									$(shell find core/ -type f -name "*.S") \
									$(shell find apic/ -type f -name "*.S")
KERNEL_SRC_C := \
									$(shell find core/ -type f -name "*.c") \
									$(shell find acpica/ -type f -name "*.c") \
									$(shell find acpi/ -type f -name "*.c") \
									$(shell find apic/ -type f -name "*.c")

KERNEL_TARGETS_S := $(patsubst %.S,$(obj)/%.o,$(KERNEL_SRC_S))
KERNEL_TARGETS_C := $(patsubst %.c,$(obj)/%.o,$(KERNEL_SRC_C))
KERNEL_TARGETS := $(KERNEL_TARGETS_S) $(KERNEL_TARGETS_C)

ALL_SRC_S := $(KERNEL_SRC_S)
ALL_SRC_C := $(KERNEL_SRC_C)
ALL_SRC := $(ALL_SRC_S) $(ALL_SRC_C)

ALL_TARGETS_S := $(KERNEL_TARGETS_S)
ALL_TARGETS_C := $(KERNEL_TARGETS_C)
ALL_TARGETS := $(ALL_TARGETS_S) $(ALL_TARGETS_C)

MULTIBOOT2_TARGETS_S := $(filter $(obj)/multiboot2/%,$(ALL_TARGETS_S))
MULTIBOOT2_TARGETS := $(MULTIBOOT2_TARGETS_S)

CORE_TARGETS_S := $(filter $(obj)/core/%,$(ALL_TARGETS_S))
CORE_TARGETS_C := $(filter $(obj)/core/%,$(ALL_TARGETS_C))
CORE_TARGETS := $(CORE_TARGETS_S) $(CORE_TARGETS_C)

ACPICA_TARGETS_C := $(filter $(obj)/acpica/%,$(ALL_TARGETS_C))
ACPICA_TARGETS := $(ACPICA_TARGETS_C)

ACPI_TARGETS_C := $(filter $(obj)/acpi/%,$(ALL_TARGETS_C))
ACPI_TARGETS := $(ACPI_TARGETS_C)

APIC_TARGETS_S := $(filter $(obj)/apic/%,$(ALL_TARGETS_S))
APIC_TARGETS_C := $(filter $(obj)/apic/%,$(ALL_TARGETS_C))
APIC_TARGETS := $(APIC_TARGETS_C) $(APIC_TARGETS_S)

KERNEL_DEPS := $(patsubst %.c,$(deps)/%.d,$(KERNEL_SRC_C)) $(patsubst %.S,$(deps)/%.d,$(KERNEL_SRC_S))

CWARN := -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls \
	-Wnested-externs -Winline -Wno-long-long -Wconversion -Wstrict-prototypes
#CDEBUG := -D DEBUG -O0
CFLAGS := $(CWARN) -O2 $(CDEBUG) -MMD -MP -D_MODULOS -static -fno-pie -mcmodel=kernel -ffreestanding -fomit-frame-pointer -fno-asynchronous-unwind-tables \
	-mno-red-zone -mno-mmx -mno-sse -mno-sse2 -c -g3 -F dwarf -I $(CURDIR)/include/

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
	qemu-system-x86_64 -s -smp 4 -serial vc -serial file:$(test)/serial -d cpu_reset,int -m 16G -monitor stdio -drive format=raw,file=$<,if=ide,media=disk

.PHONY: debuggdb
debuggdb: $(obj)/modulos-dbg
	-gdb -q -tui \
		--init-eval-command="tar rem localhost:1234" \
		--init-eval-command="la sp" \
		--init-eval-command="sy $<" \
		--init-eval-command="c"

%/:
	-mkdir -p $@

$(obj)/modulos.img: $(obj)/modulos cfg/grub.cfg COPYING COPYING.LESSER
	# Create root fs structure
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
	# Copy files
	mkdir -p $(obj)/rootfs/boot/grub/ $(obj)/rootfs/usr/share/doc/ModulOS/
	cp $< $(obj)/rootfs/boot/
	cp cfg/grub.cfg $(obj)/rootfs/boot/grub/
	cp COPYING COPYING.LESSER $(obj)/rootfs/usr/share/doc/ModulOS/
	# Create 4GiB disk
	dd if=/dev/zero of=$@ count=1024 bs=4M
	# Format it
	parted -s $@ mklabel msdos
	parted -s $@ mkpart primary ext2 1MiB 100%
	parted -s $@ set 1 boot on
	# Create grub image
	echo "configfile (hd0,msdos1)/boot/grub/grub.cfg" > $(obj)/earlyconf.cfg
	grub-mkimage -O i386-pc -o $(obj)/grub.img -p /boot/grub --config=$(obj)/earlyconf.cfg biosdisk part_msdos ext2 configfile normal multiboot2
	# Create filesystem image
	genext2fs -b 4194304 -d $(obj)/rootfs/ $(obj)/fs.img
	# dd into image file
	dd if=/usr/lib/grub/i386-pc/boot.img of=$@ count=446 bs=1 conv=notrunc status=progress
	dd if=$(obj)/grub.img of=$@ seek=1 bs=512 conv=notrunc status=progress
	dd if=$(obj)/fs.img of=$@ seek=1 bs=1M conv=notrunc status=progress

$(obj)/modulos: $(obj)/modulos-dbg | $(obj)/
	$(KERNEL_STRIP) -s -o $@ $<

$(obj)/modulos-dbg: cfg/kernel.ld $(KERNEL_TARGETS) | $(obj)/
	$(KERNEL_LD) -o $@ -T $^

$(KERNEL_DEPS):
	mkdir -p $(dir $@)
	touch $@

include $(wildcard */Makefile)
include $(wildcard $(KERNEL_DEPS))
