# Makefile - multiboot2 build makefile
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

GRUB_MODS := multiboot2 fat normal part_gpt search

ifdef DEBUG
GRUB_MODS := $(GRUB_MODS) acpi backtrace cat echo lsmmap ls lspci minicmd probe read rdmsr search_fs_file search_fs_uuid search_label serial test wrmsr
endif

ifdef BUILD_BOOT_GRAPHICSBASE
GRUB_MODS := $(GRUB_MODS) efi_gop video
endif

$(OBJ_DIR)/gpt.img: | $(OBJ_DIR)/
	truncate -s 4G $@
	parted -s $@ mklabel gpt
	parted -s $@ mkpart ESP fat32 4MiB 52MiB
	parted -s $@ set 1 esp on
	parted -s $@ mkpart rootfs ext4 52MiB 100%
	dd if=$@ bs=4M skip=1 count=1022 status=none | cmp -n 4286578688 /dev/zero -

$(OBJ_DIR)/esp.img: grub.cfg | $(OBJ_DIR)/
	truncate -s 48M $@
	mformat -v ESP_MODULOS -F -i $@
	mmd -i $@ ::/EFI
	mmd -i $@ ::/EFI/BOOT
	$(KERNEL_TOOLCHAIN_PREFIX)grub-mkstandalone \
		--format=x86_64-efi \
		--install-modules="$(GRUB_MODS)" \
		--output=$(OBJ_DIR)/BOOTX64.EFI \
		"boot/grub/grub.cfg=$<"
	mcopy -i $@ $(OBJ_DIR)/BOOTX64.EFI ::/EFI/BOOT/
