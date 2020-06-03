/*
 * Allwinner A64 System on Chip emulation
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

/*
 * The Allwinner H3 is a System on Chip containing four ARM Cortex A7
 * processor cores. Features and specifications include DDR2/DDR3 memory,
 * SD/MMC storage cards, 10/100/1000Mbit Ethernet, USB 2.0, HDMI and
 * various I/O modules.
 *
 * This implementation is based on the following datasheet:
 *
 *   https://linux-sunxi.org/File:Allwinner_H3_Datasheet_V1.2.pdf
 *
 * The latest datasheet and more info can be found on the Linux Sunxi wiki:
 *
 *   https://linux-sunxi.org/H3
 */

#ifndef HW_ARM_ALLWINNER_A64_H
#define HW_ARM_ALLWINNER_A64_H

#include "qom/object.h"
#include "hw/arm/boot.h"
#include "hw/arm/allwinner-h3.h"
#include "hw/misc/allwinner-rsb.h"
#include "target/arm/cpu.h"
#include "sysemu/block-backend.h"

/**
 * Allwinner H3 device list
 *
 * This enumeration is can be used refer to a particular device in the
 * Allwinner H3 SoC. For example, the physical memory base address for
 * each device can be found in the AwH3State object in the memmap member
 * using the device enum value as index.
 *
 * @see AwH3State
 */
enum {
    AW_A64_SRAM_A1 = AW_H3_SRAM_A1,
    AW_A64_SRAM_A2 = AW_H3_SRAM_A2,
    AW_A64_SRAM_C = AW_H3_SRAM_C,
    AW_A64_UART4 = AW_H3_COUNT,
    AW_A64_RSB
};

/** Total number of CPU cores in the A64 SoC */
#define AW_A64_NUM_CPUS      (4)

/**
 * Allwinner H3 object model
 * @{
 */

/** Object type for the Allwinner A64 SoC */
#define TYPE_AW_A64 "allwinner-a64"

/** Convert input object to Allwinner A64 state object */
#define AW_A64(obj) OBJECT_CHECK(AwA64State, (obj), TYPE_AW_A64)

/** @} */

/**
 * Allwinner H3 object
 *
 * This struct contains the state of all the devices
 * which are currently emulated by the H3 SoC code.
 */
typedef struct AwA64State {
	/* inherit devices from h3 objects */
	AwH3State h3;

	const hwaddr *a64_memmap;
    AwRSBState rsb;
} AwA64State;

/**
 * Emulate Boot ROM firmware setup functionality.
 *
 * A real Allwinner H3 SoC contains a Boot ROM
 * which is the first code that runs right after
 * the SoC is powered on. The Boot ROM is responsible
 * for loading user code (e.g. a bootloader) from any
 * of the supported external devices and writing the
 * downloaded code to internal SRAM. After loading the SoC
 * begins executing the code written to SRAM.
 *
 * This function emulates the Boot ROM by copying 32 KiB
 * of data from the given block device and writes it to
 * the start of the first internal SRAM memory.
 *
 * @s: Allwinner H3 state object pointer
 * @blk: Block backend device object pointer
 */
void allwinner_a64_bootrom_setup(AwA64State *s, BlockBackend *blk);

#endif /* HW_ARM_ALLWINNER_A64_H */
