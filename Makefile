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

DEBUG := 1

# Boot options
BUILD_MULTIBOOT2 := 1

# Optional modules
BUILD_GRAPHICSBASE := 1

KERNEL_CC := x86_64-elf-gcc
KERNEL_LD := x86_64-elf-ld
KERNEL_STRIP := x86_64-elf-strip

obj := build
incl := include
testbuild := $(obj)/test
deps := $(obj)/deps

KERNEL_INCLUDE := core
KERNEL_DEFINES := -DCORE
ifdef BUILD_MULTIBOOT2
KERNEL_INCLUDE := $(KERNEL_INCLUDE) multiboot2
KERNEL_DEFINES := $(KERNEL_DEFINES) -DMULTIBOOT2
endif
ifdef BUILD_GRAPHICSBASE
KERNEL_INCLUDE := $(KERNEL_INCLUDE) graphicsbase
KERNEL_DEFINES := $(KERNEL_DEFINES) -DGRAPHICSBASE
endif

KERNEL_SRC := $(shell find $(KERNEL_INCLUDE) -type f \( -name "*.c" -o -name "*.S" \))

KERNEL_TARGETS_S := $(filter %.o, $(patsubst %.S,$(obj)/%.o,$(KERNEL_SRC)))
KERNEL_TARGETS_C := $(filter %.o, $(patsubst %.c,$(obj)/%.o,$(KERNEL_SRC)))
KERNEL_TARGETS := $(KERNEL_TARGETS_S) $(KERNEL_TARGETS_C)
KERNEL_DEPS := $(patsubst $(obj)/%.o,$(deps)/%.d,$(KERNEL_TARGETS))

COPY_DOC_TO := $(obj)/rootfs/doc/ModulOS/
COPY_DOC := $(COPY_DOC_TO)COPYING $(COPY_DOC_TO)COPYING.LESSER

COPY_BOOT_TO := $(obj)/rootfs/boot/
COPY_BOOT := $(COPY_BOOT_TO)modulos

CWARN := -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls \
	-Wnested-externs -Winline -Wno-long-long -Wconversion -Wstrict-prototypes
ifdef DEBUG
CDEBUG := -DDEBUG -Og -g3
else
CDEBUG := -O2 -g
endif
CFLAGS := $(CDEBUG) $(CWARN) -fno-pie -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -nostartfiles -nodefaultlibs -nostdlib -fno-asynchronous-unwind-tables -fomit-frame-pointer \
	-mcmodel=kernel -ffreestanding -F dwarf -static -D_MODULOS $(KERNEL_DEFINES) -lgcc $(CDEBUG) -I $(incl)/ -MMD -MP -c

.PHONY: all
all: build

.PHONY: build
build: $(obj)/ $(obj)/modulos.img

.PHONY: index
index:
	ctags --C-kinds=+pxzL -R $(incl)/ $(KERNEL_SRC)
	echo $(KERNEL_SRC) $(shell find $(incl) -type f) > cscope.files
	cscope -q -R -b -i cscope.files

.PHONY: clean
clean:
	-rm -rd $(obj)/

.PHONY: simulateqemu
simulateqemu: $(obj)/modulos.img | $(testbuild)/
	qemu-system-x86_64 -s -smp 4 -serial vc -serial file:$(testbuild)/serial \
		-d cpu_reset,int -m 16G -monitor stdio \
		-drive format=raw,file=$<,if=ide,media=disk \
		-smbios type=0,uefi=on -bios /usr/share/qemu/OVMF.fd \
		-device VGA

.PHONY: debuggdb
debuggdb: $(obj)/modulos-dbg
	-gdb -q -tui \
		--init-eval-command="tar rem localhost:1234" \
		--init-eval-command="la sp" \
		--init-eval-command="sy $<" \
		--init-eval-command="c"

%/:
	-mkdir -p $@

$(obj)/gpt.img:
	truncate -s 4G $@
	parted -s $@ mklabel gpt
	parted -s $@ mkpart ESP fat32 4MiB 52MiB
	parted -s $@ set 1 esp on
	parted -s $@ mkpart rootfs ext4 52MiB 100%
	dd if=$@ bs=4M skip=1 count=1022 status=none | cmp -n 4286578688 /dev/zero -

$(obj)/esp.img: cfg/grub.cfg
	truncate -s 48M $@
	mformat -F -i $@
	mmd -i $@ ::/EFI
	mmd -i $@ ::/EFI/BOOT
	grub-mkstandalone \
		--format=x86_64-efi \
		--install-modules="normal part_gpt ext2 search multiboot2 efi_gop video" \
		--output=$(obj)/BOOTX64.EFI \
		"boot/grub/grub.cfg=$<"
	mcopy -i $@ $(obj)/BOOTX64.EFI ::/EFI/BOOT/

$(COPY_DOC): $(COPY_DOC_TO)%: % | $(COPY_DOC_TO)
	cp $< $|

$(COPY_BOOT): $(COPY_BOOT_TO)%: $(obj)/% | $(COPY_BOOT_TO)
	cp $< $|

$(obj)/fs.img: $(COPY_DOC) $(COPY_BOOT)
	truncate -s 4040M $@
	yes | mke2fs -L rootfs -d $(obj)/rootfs/ -t ext4 $@

$(obj)/modulos.img: $(obj)/gpt.img $(obj)/esp.img $(obj)/fs.img
	truncate -s 4G $@
	dd if=$(obj)/gpt.img of=$@ seek=0 count=1 bs=4M conv=notrunc
	dd if=$(obj)/esp.img of=$@ seek=1 count=12 bs=4M conv=notrunc
	dd if=$(obj)/fs.img of=$@ seek=13 count=1010 bs=4M conv=notrunc
	dd if=$(obj)/gpt.img of=$@ skip=1023 seek=1023 count=1 bs=4M conv=notrunc

$(obj)/modulos: $(obj)/modulos-dbg
	$(KERNEL_STRIP) -s -o $@ $<

cfg/modulos.ld: cfg/mp.ld cfg/boot.ld cfg/kernel.ld
	touch $@

$(obj)/modulos-dbg: cfg/modulos.ld $(KERNEL_TARGETS)
	$(KERNEL_LD) -o $@ -T $^

$(KERNEL_TARGETS_C): $(obj)/%.o: %.c $(deps)/%.d
	mkdir -p $(dir $@)
	$(KERNEL_CC) $(CFLAGS) -MT $@ -MF $(deps)/$*.d -o $@ $<

$(KERNEL_TARGETS_S): $(obj)/%.o: %.S $(deps)/%.d
	mkdir -p $(dir $@)
	$(KERNEL_CC) $(CFLAGS) -MT $@ -MF $(deps)/$*.d -o $@ $<

$(KERNEL_DEPS):
	mkdir -p $(dir $@)
	touch $@

include $(wildcard $(KERNEL_DEPS))
