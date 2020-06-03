/*
 * Allwinner H3 System on Chip emulation
 *
 * Copyright (C) 2020 Uncaged Coder <uncaged-coder@proton.me>
 * Copyright (C) 2019 Niek Linnenbank <nieklinnenbank@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "exec/address-spaces.h"
#include "qapi/error.h"
#include "qemu/error-report.h"
#include "qemu/module.h"
#include "qemu/units.h"
#include "hw/qdev-core.h"
#include "cpu.h"
#include "hw/sysbus.h"
#include "hw/char/serial.h"
#include "hw/misc/unimp.h"
#include "hw/usb/hcd-ehci.h"
#include "hw/loader.h"
#include "sysemu/sysemu.h"
#include "hw/arm/allwinner-a64.h"

/* A64 specific memory map */
const hwaddr allwinner_a64_memmap[] = {
    [AW_A64_SRAM_A1]   = 0x00010000,
    [AW_A64_SRAM_A2]   = 0x00044000,
    [AW_A64_SRAM_C]    = 0x00018000,
    [AW_A64_UART4]     = 0x01c29000,
    [AW_A64_RSB]       = 0x01f03400,
};

/* List of unimplemented devices */
struct AwA64Unimplemented {
    const char *device_name;
    hwaddr base;
    hwaddr size;
} a64_unimplemented[] = {
    { "n-brom",    0x00000000, 48 * KiB },
    { "s-brom",    0x00000000, 64 * KiB },
    { "uart4",     0x01c29000, 1 * KiB },
    { "r_rsb",     0x01f03400, 1 * KiB },
    { "sdram",     0x40000000, 3 * GiB }
};

/* Shared Processor Interrupts */
enum {
    AW_H3_GIC_SPI_UART4     =  4,
    AW_A64_GIC_SPI_RSB      = 39,
};

void allwinner_a64_bootrom_setup(AwA64State *s, BlockBackend *blk)
{
    const int64_t rom_size = 32 * KiB;
    g_autofree uint8_t *buffer = g_new0(uint8_t, rom_size);

    if (blk_pread(blk, 8 * KiB, buffer, rom_size) < 0) {
        error_setg(&error_fatal, "%s: failed to read BlockBackend data",
                   __func__);
        return;
    }

    rom_add_blob("allwinner-a64.bootrom", buffer, rom_size,
                  rom_size, s->a64_memmap[AW_A64_SRAM_A1],
                  NULL, NULL, NULL, NULL, false);
}

static void allwinner_a64_init(Object *obj)
{
    AwA64State *s = AW_A64(obj);

    allwinner_h3_common_init(obj, &s->h3, ARM_CPU_TYPE_NAME("cortex-a53"));
    //s->h3.memmap = allwinner_a64_memmap;
    s->a64_memmap = allwinner_a64_memmap;

    object_initialize_child(obj, "rsb", &s->rsb, TYPE_AW_RSB);
}

static void allwinner_a64_realize(DeviceState *dev, Error **errp)
{
	unsigned i;
    AwA64State *s = AW_A64(dev);

	allwinner_h3_common_realize(dev, &s->h3, errp);

    /* SRAM */
    memory_region_init_ram(&s->h3.sram_a1, OBJECT(dev), "sram A1",
                            32 * KiB, &error_abort);
    memory_region_init_ram(&s->h3.sram_a2, OBJECT(dev), "sram A2",
                            64 * KiB, &error_abort);
    memory_region_init_ram(&s->h3.sram_c, OBJECT(dev), "sram C",
                            160 * KiB, &error_abort);
    memory_region_add_subregion(get_system_memory(), s->a64_memmap[AW_A64_SRAM_A1],
                                &s->h3.sram_a1);
    memory_region_add_subregion(get_system_memory(), s->a64_memmap[AW_A64_SRAM_A2],
                                &s->h3.sram_a2);
    memory_region_add_subregion(get_system_memory(), s->a64_memmap[AW_A64_SRAM_C],
                                &s->h3.sram_c);

    /* Reduced Serial Bus */
    sysbus_realize(SYS_BUS_DEVICE(&s->rsb), &error_fatal);
    sysbus_mmio_map(SYS_BUS_DEVICE(&s->rsb), 0, s->a64_memmap[AW_A64_RSB]);
    sysbus_connect_irq(SYS_BUS_DEVICE(&s->rsb), 0,
                       qdev_get_gpio_in(DEVICE(&s->h3.gic), AW_A64_GIC_SPI_RSB));

    /* UART4 */
    serial_mm_init(get_system_memory(), s->a64_memmap[AW_A64_UART4], 2,
                   qdev_get_gpio_in(DEVICE(&s->h3.gic), AW_H3_GIC_SPI_UART4),
                   115200, serial_hd(3), DEVICE_NATIVE_ENDIAN);

    /* Unimplemented devices */
    for (i = 0; i < ARRAY_SIZE(a64_unimplemented); i++) {
        create_unimplemented_device(a64_unimplemented[i].device_name,
                                    a64_unimplemented[i].base,
                                    a64_unimplemented[i].size);
    }
}

static void allwinner_a64_class_init(ObjectClass *oc, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);

    dc->realize = allwinner_a64_realize;
    dc->user_creatable = false;
}

static const TypeInfo allwinner_a64_type_info = {
    .name = TYPE_AW_A64,
    .parent = TYPE_DEVICE,
    .instance_size = sizeof(AwA64State),
    .instance_init = allwinner_a64_init,
    .class_init = allwinner_a64_class_init,
};

static void allwinner_a64_register_types(void)
{
    type_register_static(&allwinner_a64_type_info);
}

type_init(allwinner_a64_register_types)
