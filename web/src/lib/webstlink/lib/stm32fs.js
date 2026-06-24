/* stm32fs.js
 * stm32fs flash driver class
 *
 * Copyright Devan Lai 2017
 *
 * Ported from lib/stm32fs.py in the pystlink project,
 * Copyright Pavel Revak 2015
 *
 */

import { Exception, Warning, UsbError } from './stlinkex.js';
import { Stm32 } from './stm32.js';
import {
    hex_word as H32,
    async_sleep,
    async_timeout
} from './util.js';

const FLASH_REG_BASE = 0x40023c00;
const FLASH_KEYR_REG = FLASH_REG_BASE + 0x04;
const FLASH_SR_REG = FLASH_REG_BASE + 0x0c;
const FLASH_CR_REG = FLASH_REG_BASE + 0x10;

const FLASH_CR_LOCK_BIT = 0x80000000;
const FLASH_CR_PG_BIT = 0x00000001;
const FLASH_CR_SER_BIT = 0x00000002;
const FLASH_CR_MER_BIT = 0x00000004;
const FLASH_CR_STRT_BIT = 0x00010000;
const FLASH_CR_PSIZE_X8 = 0x00000000;
const FLASH_CR_PSIZE_X16 = 0x00000100;
const FLASH_CR_PSIZE_X32 = 0x00000200;
const FLASH_CR_PSIZE_X64 = 0x00000300;
const FLASH_CR_SNB_BITINDEX = 3;

const FLASH_SR_BUSY_BIT = 0x00010000;

// PARAMS
// R0: SRC data
// R1: DST data
// R2: size
// R4: STM32_FLASH_SR
// R5: FLASH_SR_BUSY_BIT

const FLASH_WRITER_F4_CODE_X8 = new Uint8Array([
    // write:
    0x03, 0x78,  // 0x7803    // ldrh r3, [r0]
    0x0b, 0x70,  // 0x700b    // strh r3, [r1]
    // test_busy:
    0x23, 0x68,  // 0x6823    // ldr r3, [r4]
    0x2b, 0x42,  // 0x422b    // tst r3, r5
    0xfc, 0xd1,  // 0xd1fc    // bne <test_busy>
    0x00, 0x2b,  // 0x2b00    // cmp r3, //0
    0x04, 0xd1,  // 0xd104    // bne <exit>
    0x01, 0x30,  // 0x3001    // adds r0, //1
    0x01, 0x31,  // 0x3101    // adds r1, //1
    0x01, 0x3a,  // 0x3a01    // subs r2, //1
    0x00, 0x2a,  // 0x2a00    // cmp r2, //0
    0xf3, 0xd1,  // 0xd1f3    // bne <write>
    // exit:
    0x00, 0xbe,  // 0xbe00    // bkpt 0x00
]);

const FLASH_WRITER_F4_CODE_X16 = new Uint8Array([
    // write:
    0x03, 0x88,  // 0x8803    // ldrh r3, [r0]
    0x0b, 0x80,  // 0x800b    // strh r3, [r1]
    // test_busy:
    0x23, 0x68,  // 0x6823    // ldr r3, [r4]
    0x2b, 0x42,  // 0x422b    // tst r3, r5
    0xfc, 0xd1,  // 0xd1fc    // bne <test_busy>
    0x00, 0x2b,  // 0x2b00    // cmp r3, //0
    0x04, 0xd1,  // 0xd104    // bne <exit>
    0x02, 0x30,  // 0x3002    // adds r0, //2
    0x02, 0x31,  // 0x3102    // adds r1, //2
    0x02, 0x3a,  // 0x3a02    // subs r2, //2
    0x00, 0x2a,  // 0x2a00    // cmp r2, //0
    0xf3, 0xd1,  // 0xd1f3    // bne <write>
    // exit:
    0x00, 0xbe,  // 0xbe00    // bkpt 0x00
]);

const FLASH_WRITER_F4_CODE_X32 = new Uint8Array([
    // write:
    0x03, 0x68,  // 0x6803    // ldr r3, [r0]
    0x0b, 0x60,  // 0x600b    // str r3, [r1]
    // test_busy:
    0x23, 0x68,  // 0x6823    // ldr r3, [r4]
    0x2b, 0x42,  // 0x422b    // tst r3, r5
    0xfc, 0xd1,  // 0xd1fc    // bne <test_busy>
    0x00, 0x2b,  // 0x2b00    // cmp r3, //0
    0x04, 0xd1,  // 0xd104    // bne <exit>
    0x04, 0x30,  // 0x3004    // adds r0, //4
    0x04, 0x31,  // 0x3104    // adds r1, //4
    0x04, 0x3a,  // 0x3a04    // subs r2, //4
    0x00, 0x2a,  // 0x2a00    // cmp r2, //0
    0xf3, 0xd1,  // 0xd1f3    // bne <write>
    // exit:
    0x00, 0xbe,  // 0xbe00    // bkpt 0x00
]);

const VOLTAGE_DEPENDEND_PARAMS = [
    {
        'min_voltage': 2.7,
        'max_mass_erase_time': 16,
        'max_erase_time': {16: .5, 64: 1.1, 128: 2},
        'FLASH_CR_PSIZE': FLASH_CR_PSIZE_X32,
        'FLASH_WRITER_CODE': FLASH_WRITER_F4_CODE_X32,
    },
    {
        'min_voltage': 2.1,
        'max_mass_erase_time': 22,
        'max_erase_time': {16: .6, 64: 1.4, 128: 2.6},
        'FLASH_CR_PSIZE': FLASH_CR_PSIZE_X16,
        'FLASH_WRITER_CODE': FLASH_WRITER_F4_CODE_X16,
    },
    {
        'min_voltage': 1.8,
        'max_mass_erase_time': 32,
        'max_erase_time': {16: .8, 64: 2.4, 128: 4},
        'FLASH_CR_PSIZE': FLASH_CR_PSIZE_X8,
        'FLASH_WRITER_CODE': FLASH_WRITER_F4_CODE_X8,
    }
];

class Flash {
    constructor(driver, stlink, dbg) {
        this._driver = driver;
        this._stlink = stlink;
        this._dbg = dbg;
        this._params = null;
    }

    async init() {
        this._params = await this.get_voltage_dependend_params();
        await this.unlock();
    }

    async get_voltage_dependend_params() {
        await this._stlink.read_target_voltage();
        let voltage = this._stlink.target_voltage;
        let params = VOLTAGE_DEPENDEND_PARAMS.find(
            params => (voltage > params["min_voltage"])
        );
        if (params) {
            return params;
        }
        throw new Exception(`Supply voltage is ${voltage}V, but minimum for FLASH program or erase is 1.8V`);
    }

    async unlock() {
        await this._driver.core_reset_halt();
        // programming locked
        let cr_reg = await this._stlink.get_debugreg32(FLASH_CR_REG);
        if (cr_reg & FLASH_CR_LOCK_BIT) {
            // unlock keys
            await this._stlink.set_debugreg32(FLASH_KEYR_REG, 0x45670123);
            await this._stlink.set_debugreg32(FLASH_KEYR_REG, 0xcdef89ab);
        }
        cr_reg = await this._stlink.get_debugreg32(FLASH_CR_REG);
        // programming locked
        if (cr_reg & FLASH_CR_LOCK_BIT) {
            throw new Exception("Error unlocking FLASH");
        }
    }

    async lock() {
        await this._stlink.set_debugreg32(FLASH_CR_REG, FLASH_CR_LOCK_BIT);
        await this._driver.core_reset_halt();
    }

    async erase_all() {
        await this._stlink.set_debugreg32(FLASH_CR_REG, FLASH_CR_MER_BIT);
        await this._stlink.set_debugreg32(FLASH_CR_REG, (FLASH_CR_MER_BIT | FLASH_CR_STRT_BIT));
        await this.wait_busy(this._params["max_mass_erase_time"], "Erasing FLASH");
    }

    async erase_sector(sector, erase_size) {
        let flash_cr_value = FLASH_CR_SER_BIT;
        flash_cr_value |= (this._params["FLASH_CR_PSIZE"] | (sector << FLASH_CR_SNB_BITINDEX));
        await this._stlink.set_debugreg32(FLASH_CR_REG, flash_cr_value);
        await this._stlink.set_debugreg32(FLASH_CR_REG, (flash_cr_value | FLASH_CR_STRT_BIT));
        await this.wait_busy(this._params["max_erase_time"][erase_size]);
    }

    async erase_sectors(flash_start, erase_sizes, addr, size) {
        let erase_addr = flash_start;
        this._dbg.bargraph_start("Erasing FLASH", {"value_min": flash_start, "value_max": (flash_start + size)});
        let sector = 0;
        while (true) {
            for (let erase_size of erase_sizes) {
                if (addr < (erase_addr + erase_size)) {
                    this._dbg.bargraph_update({"value": erase_addr});
                    await this.erase_sector(sector, erase_size);
                }
                erase_addr += erase_size;
                if ((addr + size) < erase_addr) {
                    this._dbg.bargraph_done();
                    return;
                }
                sector += 1;
            }
        }
    }

    async init_write(sram_offset) {
        this._flash_writer_offset = sram_offset;
        this._flash_data_offset = (sram_offset + 256);
        await this._stlink.set_mem8(this._flash_writer_offset, this._params["FLASH_WRITER_CODE"]);
        // set configuration for flash writer
        await this._driver.set_reg("R4", FLASH_SR_REG);
        await this._driver.set_reg("R5", FLASH_SR_BUSY_BIT);
        // enable PG
        await this._stlink.set_debugreg32(FLASH_CR_REG, (FLASH_CR_PG_BIT | this._params["FLASH_CR_PSIZE"]));
    }

    async write(addr, block) {
        // if all data are 0xff then will be not written
        if (block.every(b => (b == 0xff))) {
            return;
        }
        await this._stlink.set_mem32(this._flash_data_offset, block);
        await this._driver.set_reg("PC", this._flash_writer_offset);
        await this._driver.set_reg("R0", this._flash_data_offset);
        await this._driver.set_reg("R1", addr);
        await this._driver.set_reg("R2", block.length);
        await this._driver.core_run();
        await this.wait_for_breakpoint(0.2);
    }

    async wait_busy(wait_time, bargraph_msg = null) {
        const end_time = (Date.now() + (wait_time * 1.5 * 1000));
        if (bargraph_msg) {
            this._dbg.bargraph_start(bargraph_msg, {
                "value_min": Date.now()/1000.0,
                "value_max": (Date.now()/1000.0 + wait_time)
            });
        }
        while (Date.now() < end_time) {
            if (bargraph_msg) {
                this._dbg.bargraph_update({"value": Date.now()/1000.0});
            }
            let status = await this._stlink.get_debugreg32(FLASH_SR_REG);
            if (!(status & FLASH_SR_BUSY_BIT)) {
                this.end_of_operation(status);
                if (bargraph_msg) {
                    this._dbg.bargraph_done();
                }
                return;
            }
            await async_sleep(wait_time / 20);
        }
        throw new Exception("Operation timeout");
    }

    async wait_for_breakpoint(wait_time) {
        const end_time = Date.now() + (wait_time * 1000);
        do {
            let dhcsr = await this._stlink.get_debugreg32(Stm32.DHCSR_REG);
            if (dhcsr & Stm32.DHCSR_STATUS_HALT_BIT) {
                break;
            }
            await async_sleep(wait_time / 20);
        } while (Date.now() < end_time);

        let sr = await this._stlink.get_debugreg32(FLASH_SR_REG);
        this.end_of_operation(sr);
    }

    end_of_operation(status) {
        if (status) {
            throw new Exception("Error writing FLASH with status (FLASH_SR) " + H32(status));
        }
    }
}

// support all STM32F MCUs with sector access access to FLASH
// (STM32F2xx, STM32F4xx) 
class Stm32FS extends Stm32 {
    async flash_erase_all() {
        this._dbg.debug("Stm32FS.flash_erase_all()");
        let flash = new Flash(this, this._stlink, this._dbg);
        await flash.init();
        await flash.erase_all();
        await flash.lock();
    }

    async flash_write(addr, data, { erase = false, verify = false, erase_sizes = null }) {
        let addr_str = (addr !== null) ? `0x${H32(addr)}` : 'None';
        this._dbg.debug(`Stm32FS.flash_write(${addr_str}, [data:${data.length}Bytes], erase=${erase}, verify=${verify}, erase_sizes=${erase_sizes})`);
        if (addr === null) {
            addr = this.FLASH_START;
        }
        if (addr % 4) {
            throw new Exception("Start address is not aligned to word");
        }
        // align data
        if (data.length % 4) {
            let padded_data = new Uint8Array(data.length + (4 - (data.length % 4)));
            data.forEach((b, i) => padded_data[i] = b);
            padded_data.fill(0xff, data.length);
            data = padded_data;
        }
        let flash = new Flash(this, this._stlink, this._dbg);
        await flash.init();
        if (erase) {
            if (erase_sizes) {
                await flash.erase_sectors(this.FLASH_START, erase_sizes, addr, data.length);
            } else {
                await flash.erase_all();
            }
        }
        this._dbg.bargraph_start("Writing FLASH", {
            "value_min": addr,
            "value_max": (addr + data.length)
        });
        await flash.init_write(Stm32FS.SRAM_START);
        while (data.length > 0) {
            this._dbg.bargraph_update({"value": addr});
            let block = data.slice(0, this._stlink.maximum_transfer_size);
            data = data.slice(this._stlink.maximum_transfer_size);
            await flash.write(addr, block);
            if (verify) {
                let flashed_data = await this._stlink.get_mem32(addr, block.length);
                let flashed_block = new Uint8Array(flashed_data.buffer, flashed_data.byteOffset, flashed_data.byteLength);
                let verified = false;
                if (flashed_block.length == block.length) {
                    verified = flashed_block.every((octet, index) => octet == block[index]);
                }
                if (!verified) {
                    throw new Exception("Verify error at block address: 0x" + H32(addr));
                }
            }
            addr += block.length;
        }
        await flash.lock();
        this._dbg.bargraph_done();
    }
}

export { Stm32FS };
