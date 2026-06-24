/* stm32.js
 * Generic STM32 CPU access and base class for STM32 CPUs
 *
 * Copyright Devan Lai 2017
 *
 * Ported from lib/stm32.py in the pystlink project,
 * Copyright Pavel Revak 2015
 *
 */

import { Exception, Warning, UsbError } from './stlinkex.js';
import {
    hex_word as H32,
    hex_octet as H8
} from './util.js';

const REGISTERS = [
    'R0', 'R1', 'R2', 'R3', 'R4', 'R5', 'R6', 'R7', 'R8', 'R9',
    'R10', 'R11', 'R12', 'SP', 'LR', 'PC', 'PSR', 'MSP', 'PSP'
];

const SRAM_START = 0x20000000;
const FLASH_START = 0x08000000;

const AIRCR_REG = 0xe000ed0c;
const DHCSR_REG = 0xe000edf0;
const DEMCR_REG = 0xe000edfc;

const AIRCR_KEY = 0x05fa0000;
const AIRCR_SYSRESETREQ_BIT = 0x00000004;
const AIRCR_SYSRESETREQ = AIRCR_KEY | AIRCR_SYSRESETREQ_BIT;

const DHCSR_KEY = 0xa05f0000;
const DHCSR_DEBUGEN_BIT = 0x00000001;
const DHCSR_HALT_BIT = 0x00000002;
const DHCSR_STEP_BIT = 0x00000004;
const DHCSR_STATUS_HALT_BIT = 0x00020000;
const DHCSR_STATUS_LOCKUP_BIT = 0x00080000;
const DHCSR_DEBUGDIS = DHCSR_KEY;
const DHCSR_DEBUGEN = DHCSR_KEY | DHCSR_DEBUGEN_BIT;
const DHCSR_HALT = DHCSR_KEY | DHCSR_DEBUGEN_BIT | DHCSR_HALT_BIT;
const DHCSR_STEP = DHCSR_KEY | DHCSR_DEBUGEN_BIT | DHCSR_STEP_BIT;

const DEMCR_RUN_AFTER_RESET = 0x00000000;
const DEMCR_HALT_AFTER_RESET = 0x00000001;

class Stm32 {
    constructor(stlink, dbg) {
        this._stlink = stlink;
        this._dbg = dbg;
        this.FLASH_START = FLASH_START;
        this.SRAM_START = SRAM_START;
    }

    is_reg(reg) {
        return REGISTERS.includes(reg.toUpperCase());
    }

    async get_reg_all() {
        let registers = {};
        for (let reg of REGISTERS) {
            registers[reg] = await this.get_reg(reg);
        }
        return registers;
    }

    get_reg(reg) {
        this._dbg.debug(`Stm32.get_reg(${reg})`);
        let index = REGISTERS.indexOf(reg.toUpperCase());
        if (index != -1) {
            return this._stlink.get_reg(index);
        }
        throw new Exception(`Wrong register name ${reg}`);
    }

    set_reg(reg, value) {
        this._dbg.debug(`Stm32.set_reg(${reg}, 0x${H32(value)})`);
        let index = REGISTERS.indexOf(reg.toUpperCase());
        if (index != -1) {
            return this._stlink.set_reg(index, value);
        }
        throw new Exception("Wrong register name");
    }

    async get_mem(addr, size) {
        this._dbg.debug(`Stm32.get_mem(0x${H32(addr)}, ${size})`);
        if (size == 0) {
            return [];
        }
        let total = 0;
        let chunks = [];
        if (addr % 4) {
            let read_size = Math.min((4 - (addr % 4)), size);
            let chunk = await this._stlink.get_mem8(addr, read_size);
            total += chunk.byteLength;
            chunks.push(chunk);
        }
        while (true) {
            this._dbg.bargraph_update({"value": total});
            let read_size = Math.min(((size - total) & 0xfffffff8), (this._stlink.maximum_transfer_size * 2));
            if (read_size == 0) {
                break;
            }
            if (read_size > 64) {
                read_size = Math.floor(read_size / 2);
                let chunk = await this._stlink.get_mem32((addr + total), read_size);
                total += chunk.byteLength;
                chunks.push(chunk);
                chunk = await this._stlink.get_mem32((addr + total), read_size);
                total += chunk.byteLength;
                chunks.push(chunk);
            } else {
                let chunk = await this._stlink.get_mem32((addr + total), read_size);
                total += chunk.byteLength;
                chunks.push(chunk);
            }
        }
        if (total < size) {
            let read_size = (size - total);
            let chunk = await this._stlink.get_mem8((addr + total), read_size);
            total += chunk.byteLength;
            chunks.push(chunk);
        }
        this._dbg.bargraph_done();

        let data = new Uint8Array(total);
        let i = 0;
        for (let chunk of chunks) {
            const u8 = (chunk instanceof Uint8Array)
                ? chunk
                : new Uint8Array(chunk.buffer, chunk.byteOffset, chunk.byteLength);
            data.set(u8, i);
            i += u8.byteLength;
        }

        return data;
    }

    async set_mem(addr, data) {
        this._dbg.debug(`Stm32.set_mem(0x${H32(addr)}, [data:${data.length}Bytes])`);
        if (data.length == 0) {
            return;
        }
        let written_size = 0;
        if (addr % 4) {
            let write_size = Math.min((4 - (addr % 4)), data.length);
            await this._stlink.set_mem8(addr, data.slice(0, write_size));
            written_size = write_size;
        }
        while (true) {
            this._dbg.bargraph_update({"value": written_size});
            let write_size = Math.min(((data.length - written_size) & 0xfffffff8), (this._stlink.maximum_transfer_size * 2));
            if (write_size == 0) {
                break;
            }
            if (write_size > 64) {
                write_size = Math.floor(write_size / 2);
                await this._stlink.set_mem32((addr + written_size), data.slice(written_size, (written_size + write_size)));
                written_size += write_size;
                await this._stlink.set_mem32((addr + written_size), data.slice(written_size, (written_size + write_size)));
                written_size += write_size;
            } else {
                await this._stlink.set_mem32((addr + written_size), data.slice(written_size, (written_size + write_size)));
                written_size += write_size;
            }
        }
        if (written_size < data.length) {
            await this._stlink.set_mem8((addr + written_size), data.slice(written_size));
        }
        this._dbg.bargraph_done();
        return;
    }

    fill_mem(addr, size, pattern) {
        if (pattern > 0xff) {
            throw new Exception("Fill pattern can by 8 bit number");
        }
        this._dbg.debug(`Stm32.fill_mem(0x${H32(addr)}, 0x${H8(pattern)}d)`);
        if (size == 0) {
            return;
        }
        const data = new Uint8Array(size).fill(pattern);
        return this.set_mem(addr, data);
    }

    core_status() {
        this._dbg.debug("Stm32.core_status()");
        return this._stlink.get_debugreg32(DHCSR_REG);
    }

    async core_reset() {
        this._dbg.debug("Stm32.core_reset()");
        await this._stlink.set_debugreg32(DEMCR_REG, DEMCR_RUN_AFTER_RESET);
        await this._stlink.set_debugreg32(AIRCR_REG, AIRCR_SYSRESETREQ);
        await this._stlink.get_debugreg32(AIRCR_REG);
    }

    async core_reset_halt() {
        this._dbg.debug("Stm32.core_reset_halt()");
        await this._stlink.set_debugreg32(DHCSR_REG, DHCSR_HALT);
        await this._stlink.set_debugreg32(DEMCR_REG, DEMCR_HALT_AFTER_RESET);
        await this._stlink.set_debugreg32(AIRCR_REG, AIRCR_SYSRESETREQ);
        await this._stlink.get_debugreg32(AIRCR_REG);
    }

    async core_halt() {
        this._dbg.debug("Stm32.core_halt()");
        await this._stlink.set_debugreg32(DHCSR_REG, DHCSR_HALT);
    }

    async core_step() {
        this._dbg.debug("Stm32.core_step()");
        await this._stlink.set_debugreg32(DHCSR_REG, DHCSR_STEP);
    }

    async core_run() {
        this._dbg.debug("Stm32.core_run()");
        await this._stlink.set_debugreg32(DHCSR_REG, DHCSR_DEBUGEN);
    }

    async core_nodebug() {
        this._dbg.debug("Stm32.core_nodebug()");
        await this._stlink.set_debugreg32(DHCSR_REG, DHCSR_DEBUGDIS);
    }

    async flash_erase_all() {
        this._dbg.debug("Stm32.flash_mass_erase()");
        throw new Exception("Erasing FLASH is not implemented for this MCU");
    }

    async flash_write(addr, data, { erase = false, verify = false, erase_sizes = null }) {
        let addr_str = (addr !== null) ? `0x{H32(addr)}` : 'None';
        this._dbg.debug(`Stm32.flash_write(${addr_str}, [data:${data.length}Bytes], erase=${erase}, verify=${verify}, erase_sizes=${erase_sizes})`);
        throw new Exception("Programing FLASH is not implemented for this MCU");
    }
}

Stm32.SRAM_START = SRAM_START;
Stm32.FLASH_START = FLASH_START;

Stm32.AIRCR_REG = AIRCR_REG;
Stm32.DHCSR_REG = DHCSR_REG;
Stm32.DEMCR_REG = DEMCR_REG;

Stm32.AIRCR_KEY = AIRCR_KEY;
Stm32.AIRCR_SYSRESETREQ_BIT = AIRCR_SYSRESETREQ_BIT;
Stm32.AIRCR_SYSRESETREQ = AIRCR_SYSRESETREQ;

Stm32.DHCSR_KEY = DHCSR_KEY;
Stm32.DHCSR_DEBUGEN_BIT = DHCSR_DEBUGEN_BIT;
Stm32.DHCSR_HALT_BIT = DHCSR_HALT_BIT;
Stm32.DHCSR_STEP_BIT = DHCSR_STEP_BIT;
Stm32.DHCSR_STATUS_HALT_BIT = DHCSR_STATUS_HALT_BIT;
Stm32.DHCSR_STATUS_LOCKUP_BIT = DHCSR_STATUS_LOCKUP_BIT;
Stm32.DHCSR_DEBUGDIS = DHCSR_DEBUGDIS;
Stm32.DHCSR_DEBUGEN = DHCSR_DEBUGEN;
Stm32.DHCSR_HALT = DHCSR_HALT;
Stm32.DHCSR_STEP = DHCSR_STEP;

Stm32.DEMCR_RUN_AFTER_RESET = DEMCR_RUN_AFTER_RESET;
Stm32.DEMCR_HALT_AFTER_RESET = DEMCR_HALT_AFTER_RESET;

export { Stm32 };
