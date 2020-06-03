/*
 * Allwinner CPU Configuration Module emulation
 *
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
#include "qemu/units.h"
#include "hw/sysbus.h"
#include "hw/irq.h"
#include "migration/vmstate.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "qemu/error-report.h"
#include "qemu/timer.h"
#include "hw/core/cpu.h"
#include "target/arm/arm-powerctl.h"
#include "target/arm/cpu.h"
#include "hw/misc/allwinner-rsb.h"
#include "trace.h"

/* RSB register offsets */
enum {
    REG_RSB_CTRL       = 0x00, /* Control */
    REG_RSB_CCR        = 0x04, /* Clork Control */
    REG_RSB_INTE       = 0x08, /* Interrupt enable */
    REG_RSB_STAT       = 0x0C, /* Interrupt status (write 1 to clear) */
	REG_RSB_DADDR0     = 0x10, /* Register address within the slave */
    REG_RSB_PMCR       = 0x28, /* PMIC init register */
    REG_RSB_CMD        = 0x2C, /* Command for next transaction */
    REG_RSB_SADDR      = 0x30, /* Slave address */
};

/* RSM register flag */
enum {
    RSB_CTRL_GLB_INTEN  = (1 << 1),
    RSB_CTRL_ABT_XFER   = (1 << 6),
    RSB_CTRL_START_XFER = (1 << 7),
    RSB_STAT_TOVER      = (1 << 0), /* Transfer over (completed). */
};

static uint64_t allwinner_rsb_read(void *opaque, hwaddr offset,
                                      unsigned size)
{
    const AwRSBState *s = AW_RSB(opaque);
    uint64_t val = 0;

    switch (offset) {
	case REG_RSB_CTRL:
		val = s->ctrl;
		break;
	case REG_RSB_STAT:
		val = s->stat;
		break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: out-of-bounds offset 0x%04x\n",
                      __func__, (uint32_t)offset);
        break;
    }

    //trace_allwinner_rsb_read(offset, val, size);

    return val;
}

static void allwinner_rsb_write(void *opaque, hwaddr offset,
                                   uint64_t val, unsigned size)
{
    AwRSBState *s = AW_RSB(opaque);

    //trace_allwinner_rsb_write(offset, val, size);

    switch (offset) {
	case REG_RSB_CTRL:
		s->ctrl = val & RSB_CTRL_GLB_INTEN;
		if (val & RSB_CTRL_START_XFER) {
			s->stat |= RSB_STAT_TOVER;
			qemu_irq_raise(s->irq);
		}
		if (val & RSB_CTRL_ABT_XFER) {
			s->stat = 0;
			qemu_irq_lower(s->irq);
		}
		break;
	case REG_RSB_STAT:
		if (val & RSB_STAT_TOVER) {
			s->stat = 0;
			qemu_irq_lower(s->irq);
		}
		break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s: out-of-bounds offset 0x%04x\n",
                      __func__, (uint32_t)offset);
        break;
    }
}

static const MemoryRegionOps allwinner_rsb_ops = {
    .read = allwinner_rsb_read,
    .write = allwinner_rsb_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = {
        .min_access_size = 4,
        .max_access_size = 4,
    },
    .impl.min_access_size = 4,
};

static void allwinner_rsb_reset(DeviceState *dev)
{
    AwRSBState *s = AW_RSB(dev);

    /* Set default values for registers */
	s->ctrl = 0;
	s->stat = 0;
}

static void allwinner_rsb_init(Object *obj)
{
    SysBusDevice *sbd = SYS_BUS_DEVICE(obj);
    AwRSBState *s = AW_RSB(obj);

    /* Memory mapping */
    memory_region_init_io(&s->iomem, OBJECT(s), &allwinner_rsb_ops, s,
                          TYPE_AW_RSB, 1 * KiB);
    sysbus_init_mmio(sbd, &s->iomem);

    sysbus_init_irq(SYS_BUS_DEVICE(s), &s->irq);
}

static const VMStateDescription allwinner_rsb_vmstate = {
    .name = "allwinner-rsb",
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (VMStateField[]) {
        VMSTATE_UINT32(ctrl, AwRSBState),
        VMSTATE_UINT32(stat, AwRSBState),
        VMSTATE_END_OF_LIST()
    }
};

static void allwinner_rsb_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);

    dc->reset = allwinner_rsb_reset;
    dc->vmsd = &allwinner_rsb_vmstate;
}

static const TypeInfo allwinner_rsb_info = {
    .name          = TYPE_AW_RSB,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_init = allwinner_rsb_init,
    .instance_size = sizeof(AwRSBState),
    .class_init    = allwinner_rsb_class_init,
};

static void allwinner_rsb_register(void)
{
    type_register_static(&allwinner_rsb_info);
}

type_init(allwinner_rsb_register)
