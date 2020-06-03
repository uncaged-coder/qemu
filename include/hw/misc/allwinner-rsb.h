/*
 * Allwinner RSB Module emulation
 *
 * Copyright (C) 2020 Uncaged Coder <uncaged-coder@proton.me>
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

#ifndef HW_MISC_ALLWINNER_RSB_H
#define HW_MISC_ALLWINNER_RSB_H

#include "qom/object.h"
#include "hw/sysbus.h"

#define TYPE_AW_RSB   "allwinner-rsb"
#define AW_RSB(obj) \
    OBJECT_CHECK(AwRSBState, (obj), TYPE_AW_RSB)

/**
 * Allwinner reduced serial bus instance state
 */
typedef struct AwRSBState {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/

    qemu_irq irq;

    MemoryRegion iomem;
	uint32_t ctrl;
	uint32_t stat;

} AwRSBState;

#endif // HW_MISC_ALLWINNER_RSB_H
