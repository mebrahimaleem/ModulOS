# Makefile - drivers build makefile
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

export SRC_TREE_ROOT := ../$(SRC_TREE_ROOT)
include $(SRC_TREE_ROOT)/scripts/Makefile.header

export OBJ_DIR := $(OBJ_DIR)/drivers

$(call add_directory,pic_8259,PIC_8259)
$(call add_directory,apic,APIC)
$(call add_directory,ioapic,IOAPIC)
$(call add_directory,acpi,ACPI)
$(call add_directory,acpica_osl,ACPICA_OSL)
$(call add_directory,pci,PCI)

ifdef BUILD_DRIVERS_HPET
$(call add_directory,hpet,HPET)
endif

#TODO: switch to PCIe

ifdef BUILD_DRIVERS_SERIAL
$(call add_directory,serial,SERIAL)
endif

include $(SRC_TREE_ROOT)/scripts/Makefile.kcflags
include $(SRC_TREE_ROOT)/scripts/Makefile.targets

.PHONY: all
all: build

.PHONY: build
build: $(OBJ_DIR)/../drivers.a $(OBJ_DIR)/../acpica.a $(LDS)

.PHONY: $(OBJ_DIR)/acpica.a
$(OBJ_DIR)/acpica.a:
	$(MAKE) -C acpica/ build

$(OBJ_DIR)/drivers.a: $(TARGETS) | $(BUILD_DIRS)
	rm -f $@
	$(AR) rcs $@ $(TARGETS)
