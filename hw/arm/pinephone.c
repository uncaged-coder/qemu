/*
 * pinephone emulation
 *
 *
 * Copyright (C) 2020 Uncaged Coder <uncaged-coder@proton.me>
 * Copyright (C) 2013 Li Guang <lig.fnst@cn.fujitsu.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include "qemu/osdep.h"
#include "qemu/units.h"
#include "exec/address-spaces.h"
#include "qapi/error.h"
#include "cpu.h"
#include "sysemu/sysemu.h"
#include "hw/sysbus.h"
#include "hw/boards.h"
#include "hw/qdev-properties.h"
#include "hw/arm/allwinner-a64.h"
#include "sysemu/sysemu.h"

static struct arm_boot_info pinephone_binfo = {
	.nb_cpus = AW_A64_NUM_CPUS,
};

static void pinephone_init(MachineState *machine)
{
    AwA64State *a64;
    DriveInfo *di;
    BlockBackend *blk;
    BusState *bus;
    DeviceState *carddev;

    /* BIOS is not supported by this board */
    if (bios_name) {
        error_report("BIOS not supported for this machine");
        exit(1);
    }

    /* This board has fixed size RAM (2GiB) */
    if (machine->ram_size != 2 * GiB) {
        error_report("This machine can only be used with 2GiB RAM");
        exit(1);
    }

    /* Only allow Cortex-A53 for this board */
    if (strcmp(machine->cpu_type, ARM_CPU_TYPE_NAME("cortex-a53")) != 0) {
        error_report("This board can only be used with cortex-a53 CPU");
        exit(1);
    }

    a64 = AW_A64(object_new(TYPE_AW_A64));
    object_property_add_child(OBJECT(machine), "soc", OBJECT(a64));
    object_unref(OBJECT(a64));

    /* Setup timer properties */
    object_property_set_int(OBJECT(a64), 32768, "clk0-freq",
                            &error_abort);
    object_property_set_int(OBJECT(a64), 24 * 1000 * 1000, "clk1-freq",
                            &error_abort);

    /* Setup SID properties. Currently using a default fixed SID identifier. */
    if (qemu_uuid_is_null(&a64->h3.sid.identifier)) {
        qdev_prop_set_string(DEVICE(a64), "identifier",
                             "02c00081-1111-2222-3333-000044556677");
    } else if (ldl_be_p(&a64->h3.sid.identifier.data[0]) != 0x02c00081) {
        warn_report("Security Identifier value does not include A64 prefix");
    }

    /* Setup EMAC properties */
    object_property_set_int(OBJECT(&a64->h3.emac), 1, "phy-addr", &error_abort);

    /* DRAMC */
    object_property_set_uint(OBJECT(a64), a64->h3.memmap[AW_H3_SDRAM],
                             "ram-addr", &error_abort);
    object_property_set_int(OBJECT(a64), machine->ram_size / MiB, "ram-size",
                            &error_abort);

    /* Mark A64 object realized */
    object_property_set_bool(OBJECT(a64), true, "realized", &error_abort);

    /* Retrieve SD bus */
    di = drive_get_next(IF_SD);
    blk = di ? blk_by_legacy_dinfo(di) : NULL;
    bus = qdev_get_child_bus(DEVICE(a64), "sd-bus");

    /* Plug in SD card */
    carddev = qdev_new(TYPE_SD_CARD);
    qdev_prop_set_drive_err(carddev, "drive", blk, &error_fatal);
    qdev_realize_and_unref(carddev, bus, &error_fatal);

    /* SDRAM */
    memory_region_add_subregion(get_system_memory(), a64->h3.memmap[AW_H3_SDRAM],
                                machine->ram);

    /* Load target kernel or start using BootROM */
    if (!machine->kernel_filename && blk && blk_is_available(blk)) {
        /* Use Boot ROM to copy data from SD card to SRAM */
        allwinner_a64_bootrom_setup(a64, blk);
    }

    pinephone_binfo.loader_start = a64->h3.memmap[AW_H3_SDRAM];
    pinephone_binfo.ram_size = machine->ram_size;
    arm_load_kernel(ARM_CPU(first_cpu), machine, &pinephone_binfo);
	printf("qemu pinephone init done\n");
}

static void pinephone_machine_init(MachineClass *mc)
{
    mc->desc = "pinephone (Cortex-A53)";
    mc->default_cpu_type = ARM_CPU_TYPE_NAME("cortex-a53");
    mc->min_cpus = AW_A64_NUM_CPUS;
    mc->max_cpus = AW_A64_NUM_CPUS;
    mc->default_cpus = AW_A64_NUM_CPUS;
    mc->default_ram_size = 2 * GiB;
    mc->init = pinephone_init;
    mc->block_default_type = IF_SD;
    mc->units_per_default_bus = 1;
    mc->default_ram_id = "pinephone.ram";
}

DEFINE_MACHINE("pinephone", pinephone_machine_init)
