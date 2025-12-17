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

$(info $(SRC_TREE_ROOT) $(OBJ_DIR))

$(OBJ_DIR)/gpt.img: | $(OBJ_DIR)/
	truncate -s 4G $@
	parted -s $@ mklabel gpt
	parted -s $@ mkpart ESP fat32 4MiB 52MiB
	parted -s $@ set 1 esp on
	parted -s $@ mkpart rootfs ext4 52MiB 100%
	dd if=$@ bs=4M skip=1 count=1022 status=none | cmp -n 4286578688 /dev/zero -

$(OBJ_DIR)/esp.img: grub.cfg | $(OBJ_DIR)/
	truncate -s 48M $@
	mformat -F -i $@
	mmd -i $@ ::/EFI
	mmd -i $@ ::/EFI/BOOT
	x86_64-elf-grub-mkstandalone \
		--format=x86_64-efi \
		--install-modules="normal part_gpt ext2 search multiboot2 efi_gop video" \
		--output=$(OBJ_DIR)/BOOTX64.EFI \
		"boot/grub/grub.cfg=$<"
	mcopy -i $@ $(OBJ_DIR)/BOOTX64.EFI ::/EFI/BOOT/
