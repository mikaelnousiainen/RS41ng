/* stm32fp.js
 * stm32 flash driver classes for parts with flash page-level access
 *
 * Copyright Devan Lai 2017
 *
 * Ported from lib/stm32fp.py in the pystlink project,
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
import { readMemU32, writeMemU32, writeMemU16 } from './mem-bus.js';

const FLASH_REG_BASE = 0x40022000;
const FLASH_REG_BASE_STEP = 0x40;
const FLASH_KEYR_INDEX = 0x04;
const FLASH_SR_INDEX = 0x0c;
const FLASH_CR_INDEX = 0x10;
const FLASH_AR_INDEX = 0x14;
const FLASH_KEYR_REG = FLASH_REG_BASE + FLASH_KEYR_INDEX;
const FLASH_SR_REG = FLASH_REG_BASE + FLASH_SR_INDEX;
const FLASH_CR_REG = FLASH_REG_BASE + FLASH_CR_INDEX;
const FLASH_AR_REG = FLASH_REG_BASE + FLASH_AR_INDEX;

const FLASH_CR_LOCK_BIT = 0x00000080;
const FLASH_CR_PG_BIT = 0x00000001;
const FLASH_CR_PER_BIT = 0x00000002;
const FLASH_CR_MER_BIT = 0x00000004;
const FLASH_CR_STRT_BIT = 0x00000040;
const FLASH_SR_BUSY_BIT = 0x00000001;
const FLASH_SR_PGERR_BIT = 0x00000004;
const FLASH_SR_WRPRTERR_BIT = 0x00000010;
const FLASH_SR_EOP_BIT = 0x00000020;

// Option byte registers (STM32F0xx/F1xx/F3xx)
const FLASH_OPTKEYR_REG = FLASH_REG_BASE + 0x08;
const FLASH_OBR_REG     = FLASH_REG_BASE + 0x1c;
const FLASH_WRPR_REG    = FLASH_REG_BASE + 0x20;

// FLASH_CR bits for option byte operations
const FLASH_CR_OPTPG_BIT  = 0x00000010;  // bit 4: Option byte programming
const FLASH_CR_OPTER_BIT  = 0x00000020;  // bit 5: Option byte erase
const FLASH_CR_OPTWRE_BIT = 0x00000200;  // bit 9: Option byte write enable (set after OPTKEYR unlock)

const FLASH_OBR_RDPRT_BIT = 0x00000002;  // bit 1: Read protection active

// Option byte addresses in system memory (STM32F1xx)
// Each 32-bit word packs two option half-words: [byte0, ~byte0, byte1, ~byte1]
const OPT_RDP_USER_ADDR = 0x1ffff800;  // RDP + USER
const OPT_DATA_ADDR     = 0x1ffff804;  // Data0 + Data1
const OPT_WRP01_ADDR    = 0x1ffff808;  // WRP0 + WRP1
const OPT_WRP23_ADDR    = 0x1ffff80c;  // WRP2 + WRP3

// PARAMS (passed in CPU registers by Flash.write / init_write)
// R0: SRC in SRAM (halfword stream)
// R1: DST in FLASH (current program address)
// R2: remaining size in bytes (decremented by 2 per halfword)
// R4: address of FLASH_SR register
// R5: FLASH_SR_BUSY_BIT (0x01) — poll until BSY clears
// R6: FLASH_SR_EOP_BIT (0x20) — expect set after each program; must clear before next halfword
//
// Behavior vs legacy pystlink payload:
// - Original loop cleared EOP only implicitly; STM32F1 requires writing 1 to EOP (W1C) before
//   starting the next halfword program. We subs r2 / branch first so the final halfword leaves
//   EOP set for wait_for_breakpoint + end_of_operation; only when more data remains do we
//   movs r7,#0x20; str r7,[r4], then bump r0/r1 and branch to the next strh.
const FLASH_WRITER_F0_CODE = new Uint8Array([
    // write:
    0x03, 0x88,  // 0x8803    // ldrh r3, [r0, #0]
    0x0b, 0x80,  // 0x800b    // strh r3, [r1, #0]
    // test_busy:
    0x23, 0x68,  // 0x6823    // ldr r3, [r4, #0]
    0x2b, 0x42,  // 0x422b    // tst r3, r5
    0xfc, 0xd1,  // 0xd1fc    // bne <test_busy>
    0x33, 0x42,  // 0x4233    // tst r3, r6
    0x08, 0xd0,  // 0xd008    // beq <exit_err>
    0x02, 0x3a,  // 0x3a02    // subs r2, #2
    0x00, 0x2a,  // 0x2a00    // cmp r2, #0
    0x04, 0xd0,  // 0xd004    // beq <exit_ok>
    // more halfwords: clear EOP then advance (F1 programming manual)
    0x20, 0x27,  // 0x2720    // movs r7, #0x20
    0x27, 0x60,  // 0x6027    // str r7, [r4, #0]
    0x02, 0x30,  // 0x3002    // adds r0, #2
    0x02, 0x31,  // 0x3102    // adds r1, #2
    0xf0, 0xe7,  // 0xe7f0    // b <write>
    // exit_ok:
    0x00, 0xbe,  // 0xbe00    // bkpt #0
    // exit_err:
    0x00, 0xbe,  // 0xbe00    // bkpt #0
]);

/** Read flash/RAM for verify using only 8-bit reads (avoids 32-bit path / buffer-view quirks). */
async function readBytesMem8(stlink, addr, len) {
    const out = new Uint8Array(len);
    for (let off = 0; off < len; ) {
        const n = Math.min(64, len - off);
        const chunk = await stlink.get_mem8(addr + off, n);
        const u8 = new Uint8Array(chunk.buffer, chunk.byteOffset, chunk.byteLength);
        out.set(u8, off);
        off += u8.byteLength;
    }
    return out;
}

class Flash {
    constructor(driver, stlink, dbg, bank = 0) {
        this._driver = driver;
        this._stlink = stlink;
        this._dbg = dbg;

        let reg_bank = (FLASH_REG_BASE + (FLASH_REG_BASE_STEP * bank));
        this.FLASH_KEYR_REG = (reg_bank + FLASH_KEYR_INDEX);
        this.FLASH_SR_REG = (reg_bank + FLASH_SR_INDEX);
        this.FLASH_CR_REG = (reg_bank + FLASH_CR_INDEX);
        this.FLASH_AR_REG = (reg_bank + FLASH_AR_INDEX);
    }

    async init() {
        await this._stlink.read_target_voltage();
        if (this._stlink.target_voltage < 2.0) {
            throw new Exception(`Supply voltage is ${this._stlink.target_voltage.toFixed(2)}V, but minimum for FLASH program or erase is 2.0V`);
        }
        await this.unlock();
    }

    async unlock() {
        await this._driver.core_reset_halt();
        let sr = await readMemU32(this._stlink, this.FLASH_SR_REG);
        await writeMemU32(this._stlink, this.FLASH_SR_REG, sr);
        // programming locked
        let cr_reg = await readMemU32(this._stlink, this.FLASH_CR_REG);
        if (cr_reg & FLASH_CR_LOCK_BIT) {
            // unlock keys
            await writeMemU32(this._stlink, this.FLASH_KEYR_REG, 0x45670123);
            await writeMemU32(this._stlink, this.FLASH_KEYR_REG, 0xcdef89ab);
        }
        cr_reg = await readMemU32(this._stlink, this.FLASH_CR_REG);
        // programming locked
        if (cr_reg & FLASH_CR_LOCK_BIT) {
            throw new Exception("Error unlocking FLASH");
        }
    }

    async lock() {
        await writeMemU32(this._stlink, this.FLASH_CR_REG, FLASH_CR_LOCK_BIT);
        await this._driver.core_reset_halt();
    }

    async erase_all() {
        await writeMemU32(this._stlink, this.FLASH_CR_REG, FLASH_CR_MER_BIT);
        await writeMemU32(this._stlink, this.FLASH_CR_REG, (FLASH_CR_MER_BIT | FLASH_CR_STRT_BIT));
        await this.wait_busy(2, "Erasing FLASH");
    }

    async erase_page(page_addr) {
        await writeMemU32(this._stlink, this.FLASH_CR_REG, FLASH_CR_PER_BIT);
        await writeMemU32(this._stlink, this.FLASH_AR_REG, page_addr);
        await writeMemU32(this._stlink, this.FLASH_CR_REG, (FLASH_CR_PER_BIT | FLASH_CR_STRT_BIT));
        await this.wait_busy(0.2);
    }

    async erase_pages(flash_start, erase_sizes, addr, size) {
        if (size === undefined) {
            throw new Exception("erase size is undefined");
        }
        let page_addr = flash_start;
        this._dbg.bargraph_start("Erasing FLASH", {"value_min": addr, "value_max": (addr + size)});
        while (true) {
            for (let page_size of erase_sizes) {
                if (addr < (page_addr + page_size)) {
                    this._dbg.bargraph_update({"value": page_addr});
                    await this.erase_page(page_addr);
                }
                page_addr += page_size;
                if ((addr + size) < page_addr) {
                    this._dbg.bargraph_done();
                    return;
                }
            }
        }
    }

    async init_write(sram_offset) {
        this._dbg.debug("Loading flash algorithm at 0x" + H32(sram_offset));
        this._flash_writer_offset = sram_offset;
        this._flash_data_offset = (sram_offset + 256);
        await this._stlink.set_mem8(this._flash_writer_offset, FLASH_WRITER_F0_CODE);
        // set configuration for flash writer
        await this._driver.set_reg("R4", this.FLASH_SR_REG);
        await this._driver.set_reg("R5", FLASH_SR_BUSY_BIT);
        await this._driver.set_reg("R6", FLASH_SR_EOP_BIT);
        // enable PG
        await writeMemU32(this._stlink, this.FLASH_CR_REG, FLASH_CR_PG_BIT);
    }

    async write(addr, block) {
        // if all data are 0xff then will be not written
        if (block.every(b => (b == 0xff))) {
            return;
        }

        this._dbg.debug(`Flashing ${block.length} bytes at 0x${H32(addr)}`);
        try {
            this._dbg.debug("Loading working buffer at 0x" + H32(this._flash_data_offset));
            await this._stlink.set_mem32(this._flash_data_offset, block);
            // Cortex-M3: start Thumb code with PC LSB set (debugger may not set EPSR.T otherwise).
            await this._driver.set_reg("PC", this._flash_writer_offset | 1);
            await this._driver.set_reg("R0", this._flash_data_offset);
            await this._driver.set_reg("R1", addr);
            await this._driver.set_reg("R2", block.length);
            await this._driver.core_run();
            await this.wait_for_breakpoint(0.2);
        } catch (e) {
            throw new Exception(`Failed to flash ${block.length} bytes at 0x${H32(addr)}: ` + e);
        }
    }

    async wait_busy(wait_time, bargraph_msg = null) {
        if (bargraph_msg) {
            this._dbg.bargraph_start(bargraph_msg, {
                "value_min": Date.now()/1000.0,
                "value_max": (Date.now()/1000.0 + wait_time)
            });
        }

        // all times are from data sheet, will be more safe to wait 2 time longer
        const end_time = (Date.now() + (wait_time * 2 * 1000));
        while (Date.now() < end_time) {
            if (bargraph_msg) {
                this._dbg.bargraph_update({"value": Date.now()/1000.0});
            }
            let status = await readMemU32(this._stlink, this.FLASH_SR_REG);
            if (!(status & FLASH_SR_BUSY_BIT)) {
                await this.end_of_operation(status);
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
            let dhcsr;
            try {
                dhcsr = await readMemU32(this._stlink, Stm32.DHCSR_REG);
            } catch (e) {
                // Transient USB short read during flash operation — retry
                this._dbg.debug("DHCSR read failed (retrying): " + e);
                await async_sleep(wait_time / 20);
                continue;
            }
            this._dbg.debug("DHCSR = 0x" + H32(dhcsr));
            if (dhcsr & Stm32.DHCSR_STATUS_LOCKUP_BIT) {
                throw new Exception("Flash algorithm crashed");
            }
            if (dhcsr & Stm32.DHCSR_STATUS_HALT_BIT) {
                break;
            }
            await async_sleep(wait_time / 20);
        } while (Date.now() < end_time);

        let sr = await readMemU32(this._stlink, this.FLASH_SR_REG);
        await this.end_of_operation(sr);
    }

    async end_of_operation(status) {
        if (status & (FLASH_SR_PGERR_BIT | FLASH_SR_WRPRTERR_BIT)) {
            throw new Exception("Error writing FLASH with status (FLASH_SR) " + H32(status));
        }
        if (status & FLASH_SR_BUSY_BIT) {
            throw new Exception("Flash still busy (FLASH_SR) " + H32(status));
        }
        if (status & FLASH_SR_EOP_BIT) {
            await writeMemU32(this._stlink, this.FLASH_SR_REG, FLASH_SR_EOP_BIT);
            return;
        }
        if (status === 0) {
            return;
        }
        throw new Exception("Error writing FLASH with status (FLASH_SR) " + H32(status));
    }
}

// support all STM32F MCUs with page access to FLASH
// (STM32F0xx, STM32F1xx and also STM32F3xx)
class Stm32FP extends Stm32 {
    async _flash_erase_all(bank = 0) {
        var flash;
        flash = new Flash(this, this._stlink, this._dbg, bank);
        await flash.init();
        await flash.erase_all();
        await flash.lock();
    }
    
    async flash_erase_all() {
        this._dbg.debug("Stm32FP.flash_erase_all()");
        await this._flash_erase_all();
    }

    async _flash_write(addr, data, { erase = false, verify = false, erase_sizes = null, bank = 0 }) {
        // align data
        if (data.length % 4) {
            let padded_data = new Uint8Array(data.length + (4 - (data.length % 4)));
            data.forEach((b, i) => padded_data[i] = b);
            padded_data.fill(0xff, data.length);
            data = padded_data;
        }
        let flash = new Flash(this, this._stlink, this._dbg, bank);
        await flash.init();
        if (erase) {
            if (erase_sizes) {
                await flash.erase_pages(this.FLASH_START, erase_sizes, addr, data.length);
            } else {
                await flash.erase_all();
            }
        }
        this._dbg.bargraph_start("Writing FLASH", {"value_min": addr, "value_max": (addr + data.length)});
        await flash.init_write(Stm32FP.SRAM_START);
        while (data.length > 0) {
            this._dbg.bargraph_update({"value": addr});
            let block = data.slice(0, this._stlink.maximum_transfer_size);
            data = data.slice(this._stlink.maximum_transfer_size);
            await flash.write(addr, block);
            if (verify) {
                this._dbg.debug(`Verifying ${block.length} bytes`);
                // Clear PG before read-back: with PG=1 the F1 flash interface can return
                // inconsistent data on debug reads while programming mode is active.
                await writeMemU32(this._stlink, flash.FLASH_CR_REG, 0);
                try {
                    let flashed_block = await readBytesMem8(this._stlink, addr, block.length);
                    let verified = false;
                    let firstDiff = -1;
                    if (flashed_block.length == block.length) {
                        for (let i = 0; i < block.length; i++) {
                            if (flashed_block[i] !== block[i]) {
                                firstDiff = i;
                                break;
                            }
                        }
                        verified = firstDiff < 0;
                    } else {
                        firstDiff = -2;
                    }
                    if (!verified) {
                        const encHex = (u8, n) => Array.from(u8.subarray(0, n)).map((x) => x.toString(16).padStart(2, "0")).join(" ");
                        const detail = firstDiff >= 0
                            ? ` firstMismatch@+${firstDiff} exp=0x${block[firstDiff].toString(16)} got=0x${flashed_block[firstDiff].toString(16)}`
                            : (firstDiff === -2 ? ` lenMismatch flashView=${flashed_block.length}` : "");
                        const fullMsg = "Verify error at block address: 0x" + H32(addr) +
                            detail + ` exp16=${encHex(block, 16)} rd16=${encHex(flashed_block, 16)}`;
                        this._dbg.error(`[flash verify] bank=${bank} blockLen=${block.length} ${fullMsg}`);
                        throw new Exception(fullMsg);
                    }
                } finally {
                    if (data.length > 0) {
                        await writeMemU32(this._stlink, flash.FLASH_CR_REG, FLASH_CR_PG_BIT);
                    }
                }
            }
            addr += block.length;
        }
        await flash.lock();
        this._dbg.bargraph_done();
    }
    async flash_write(addr, data, { erase = false, verify = false, erase_sizes = null }) {
        let addr_str = (addr !== null) ? `0x{H32(addr)}` : 'None';
        this._dbg.debug(`Stm32FP.flash_write(${addr_str}, [data:${data.length}Bytes], erase=${erase}, verify=${verify}, erase_sizes=${erase_sizes})`);
        if (addr === null) {
            addr = this.FLASH_START;
        } else {
            if (addr % 2) {
                throw new Exception("Start address is not aligned to half-word");
            }
        }
        await this._flash_write(addr, data, arguments[2]);
    }

    // --- Option byte / flash protection support ---

    /**
     * Read flash protection status (RDP + per-page write protection).
     * Returns { rdp_active: boolean, wrpr_raw: number }.
     *
     * F1/F3: Level 0 at 0x1FFFF800 is RDP=0xA5, nRDP=0x5A (little-endian halfwords).
     * F0: Level 0 uses RDP=0xAA, nRDP=0x55 (RM0410).
     * When option bytes read as erased (0xFFFFFFFF), fall back to FLASH_OBR RDPRT.
     */
    async read_flash_protection(cpuType = '') {
        this._dbg.debug("Stm32FP.read_flash_protection()");
        await this.core_halt();
        const obr = await readMemU32(this._stlink, FLASH_OBR_REG);
        const wrpr = await readMemU32(this._stlink, FLASH_WRPR_REG);
        const optWord = await readMemU32(this._stlink, OPT_RDP_USER_ADDR);
        const rdpB = optWord & 0xff;
        const nrdpB = (optWord >> 8) & 0xff;
        const level0F1 = rdpB === 0xa5 && nrdpB === 0x5a;
        const level0F0 = rdpB === 0xaa && nrdpB === 0x55;
        const level0 = level0F1 || level0F0;
        const optUnreadable = optWord === 0xffffffff || optWord === 0;
        let rdp_active;
        if (optUnreadable) {
            rdp_active = (obr & FLASH_OBR_RDPRT_BIT) !== 0;
        } else {
            rdp_active = !level0;
        }
        return {
            type: "f1",
            rdp_active,
            wrpr_raw: wrpr,
        };
    }

    /**
     * Remove all flash protection by erasing option bytes and reprogramming
     * with RDP=Level 0 (0xA5), WRP=all unprotected (0xFF).
     * Triggers a system reset to reload option bytes.
     *
     * WARNING: If RDP Level 1 was active, erasing option bytes triggers a
     * mass erase of the entire main flash. This is STM32 hardware behavior.
     */
    async remove_flash_protection(cpuType = '') {
        this._dbg.debug("Stm32FP.remove_flash_protection()");

        /** F0 Level-0 RDP byte 0xAA; F1/F3/F100 use 0xA5 (PM0075). */
        const rdpLevel0 = cpuType.startsWith('STM32F0') ? 0xaa : 0xa5;

        // Step 1: Halt the core
        await this.core_reset_halt();

        // Step 2: Unlock main flash (AHB — not DAP debug-register file)
        let cr_reg = await readMemU32(this._stlink, FLASH_CR_REG);
        if (cr_reg & FLASH_CR_LOCK_BIT) {
            await writeMemU32(this._stlink, FLASH_KEYR_REG, 0x45670123);
            await writeMemU32(this._stlink, FLASH_KEYR_REG, 0xcdef89ab);
        }
        cr_reg = await readMemU32(this._stlink, FLASH_CR_REG);
        if (cr_reg & FLASH_CR_LOCK_BIT) {
            throw new Exception("Failed to unlock flash for option byte access");
        }

        // Step 3: Unlock option bytes (same keys, sets OPTWRE bit)
        await writeMemU32(this._stlink, FLASH_OPTKEYR_REG, 0x45670123);
        await writeMemU32(this._stlink, FLASH_OPTKEYR_REG, 0xcdef89ab);
        cr_reg = await readMemU32(this._stlink, FLASH_CR_REG);
        if (!(cr_reg & FLASH_CR_OPTWRE_BIT)) {
            throw new Exception("Failed to unlock option bytes (OPTWRE not set)");
        }

        // Clear all sticky SR flags before starting (leftover PGERR/EOP from previous ops)
        await this._clear_flash_sr();

        // Step 4: Erase all option bytes (OPTER + STRT)
        await writeMemU32(this._stlink, FLASH_CR_REG, FLASH_CR_OPTER_BIT | FLASH_CR_OPTWRE_BIT);
        await writeMemU32(this._stlink, FLASH_CR_REG, FLASH_CR_OPTER_BIT | FLASH_CR_OPTWRE_BIT | FLASH_CR_STRT_BIT);
        await this._wait_flash_busy_no_throw(2.0);

        // Clear SR flags after erase, clear OPTER before programming (PM0075: OPTER and OPTPG are mutually exclusive)
        await this._clear_flash_sr();

        // Step 5: Program option bytes with protection disabled.
        //
        // ST-Link only supports 32-bit memory writes. Each 32-bit-aligned option byte word
        // contains two option halfwords. Writing them individually via writeMemU16 doesn't work
        // because the second halfword's 32-bit write overwrites the first halfword (the FPEC
        // re-programs both halfwords in the 32-bit word).
        //
        // Solution: write each 32-bit option word as a single write containing both halfwords
        // packed as [byte0, ~byte0, byte1, ~byte1] (little-endian).

        function packOptWord(b0, b1) {
            // Each option byte is stored with its complement: [byte, ~byte]
            // Two option bytes per 32-bit word: [b0, ~b0, b1, ~b1]
            return ((~b1 & 0xff) << 24) | ((b1 & 0xff) << 16) | ((~b0 & 0xff) << 8) | (b0 & 0xff);
        }

        const rdpByte = rdpLevel0;      // 0xA5 for F1, 0xAA for F0
        const userByte = 0xff;           // Default USER byte

        const optWords = [
            [OPT_RDP_USER_ADDR,  packOptWord(rdpByte, userByte)],  // RDP + USER
            [OPT_DATA_ADDR,      packOptWord(0xff, 0xff)],          // Data0 + Data1
            [OPT_WRP01_ADDR,     packOptWord(0xff, 0xff)],          // WRP0 + WRP1
            [OPT_WRP23_ADDR,     packOptWord(0xff, 0xff)],          // WRP2 + WRP3
        ];

        for (const [addr, value] of optWords) {
            await writeMemU32(this._stlink, FLASH_CR_REG, FLASH_CR_OPTPG_BIT | FLASH_CR_OPTWRE_BIT);
            await writeMemU32(this._stlink, addr, value);
            await this._wait_flash_busy(0.5);
            await this._clear_flash_sr();
        }

        // Step 6: End option programming, lock FPEC
        await writeMemU32(this._stlink, FLASH_CR_REG, FLASH_CR_OPTWRE_BIT);
        await writeMemU32(this._stlink, FLASH_CR_REG, FLASH_CR_LOCK_BIT);

        // Step 7: System reset to reload option bytes
        // STM32F1 has no OBL_LAUNCH bit — must use AIRCR.SYSRESETREQ
        await this.core_reset();
    }

    /** Clear all sticky status flags (PGERR, WRPRTERR, EOP) in FLASH_SR. W1C register. */
    async _clear_flash_sr() {
        let sr = await readMemU32(this._stlink, FLASH_SR_REG);
        const clr = sr & (FLASH_SR_PGERR_BIT | FLASH_SR_WRPRTERR_BIT | FLASH_SR_EOP_BIT);
        if (clr) {
            await writeMemU32(this._stlink, FLASH_SR_REG, clr);
        }
    }

    /** Wait for BSY to clear, then throw on PGERR/WRPRTERR. */
    async _wait_flash_busy(timeout_sec) {
        const end_time = Date.now() + (timeout_sec * 2 * 1000);
        while (Date.now() < end_time) {
            let sr = await readMemU32(this._stlink, FLASH_SR_REG);
            if (!(sr & FLASH_SR_BUSY_BIT)) {
                if (sr & FLASH_SR_PGERR_BIT) {
                    throw new Exception("Flash programming error (PGERR) during option byte operation");
                }
                if (sr & FLASH_SR_WRPRTERR_BIT) {
                    throw new Exception("Write protection error (WRPRTERR) during option byte operation");
                }
                return;
            }
            await async_sleep(timeout_sec / 20);
        }
        throw new Exception("Timeout waiting for flash operation to complete");
    }

    /**
     * Enable flash read protection (RDP Level 1).
     * Programs option bytes with RDP = 0x00 (Level 1 for F1/F3).
     *
     * SAFETY: Level 1 (RDP = 0x00) is reversible. Level 2 (RDP = 0xCC) is
     * permanent and irreversible — we NEVER write 0xCC.
     *
     * Going Level 0 → Level 1 does NOT trigger mass erase (safe).
     * NOTE: F1 requires a POWER CYCLE for RDP changes to take effect.
     */
    async enable_flash_protection(_cpuType = '') {
        // _cpuType is accepted for signature parity with remove_flash_protection
        // (the caller passes it uniformly); F1 enable needs no per-chip branching.
        this._dbg.debug("Stm32FP.enable_flash_protection()");

        const RDP_LEVEL1 = 0x00;

        // SAFETY ASSERTION: Never write Level 2
        if (RDP_LEVEL1 === 0xcc || RDP_LEVEL1 === 0xCC) {
            throw new Exception("SAFETY: Refusing to write RDP Level 2 (0xCC) — this is irreversible!");
        }

        // Step 1: Halt the core
        await this.core_reset_halt();

        // Step 2: Unlock main flash
        let cr_reg = await readMemU32(this._stlink, FLASH_CR_REG);
        if (cr_reg & FLASH_CR_LOCK_BIT) {
            await writeMemU32(this._stlink, FLASH_KEYR_REG, 0x45670123);
            await writeMemU32(this._stlink, FLASH_KEYR_REG, 0xcdef89ab);
        }
        cr_reg = await readMemU32(this._stlink, FLASH_CR_REG);
        if (cr_reg & FLASH_CR_LOCK_BIT) {
            throw new Exception("Failed to unlock flash for option byte access");
        }

        // Step 3: Unlock option bytes
        await writeMemU32(this._stlink, FLASH_OPTKEYR_REG, 0x45670123);
        await writeMemU32(this._stlink, FLASH_OPTKEYR_REG, 0xcdef89ab);
        cr_reg = await readMemU32(this._stlink, FLASH_CR_REG);
        if (!(cr_reg & FLASH_CR_OPTWRE_BIT)) {
            throw new Exception("Failed to unlock option bytes (OPTWRE not set)");
        }

        // Clear all sticky SR flags
        await this._clear_flash_sr();

        // Step 4: Erase all option bytes (OPTER + STRT)
        await writeMemU32(this._stlink, FLASH_CR_REG, FLASH_CR_OPTER_BIT | FLASH_CR_OPTWRE_BIT);
        await writeMemU32(this._stlink, FLASH_CR_REG, FLASH_CR_OPTER_BIT | FLASH_CR_OPTWRE_BIT | FLASH_CR_STRT_BIT);
        await this._wait_flash_busy_no_throw(2.0);

        // Clear SR flags after erase
        await this._clear_flash_sr();

        // Step 5: Program option bytes with RDP = 0x00 (Level 1)
        // Each F1 option byte is stored with its complement ([byte, ~byte]); two
        // bytes per 32-bit word as [b0, ~b0, b1, ~b1]. Hardware rejects mismatches.
        function packOptWord(b0, b1) {
            return ((~b1 & 0xff) << 24) | ((b1 & 0xff) << 16) | ((~b0 & 0xff) << 8) | (b0 & 0xff);
        }

        const optWords = [
            [OPT_RDP_USER_ADDR, packOptWord(RDP_LEVEL1, 0xff)],  // RDP=0x00 (Level 1) + USER=0xFF
            [OPT_DATA_ADDR,     packOptWord(0xff, 0xff)],          // Data0 + Data1
            [OPT_WRP01_ADDR,    packOptWord(0xff, 0xff)],          // WRP0 + WRP1 (unprotected)
            [OPT_WRP23_ADDR,    packOptWord(0xff, 0xff)],          // WRP2 + WRP3 (unprotected)
        ];

        for (const [addr, value] of optWords) {
            await writeMemU32(this._stlink, FLASH_CR_REG, FLASH_CR_OPTPG_BIT | FLASH_CR_OPTWRE_BIT);
            await writeMemU32(this._stlink, addr, value);
            await this._wait_flash_busy(0.5);
            await this._clear_flash_sr();
        }

        // Step 6: Lock flash
        await writeMemU32(this._stlink, FLASH_CR_REG, FLASH_CR_OPTWRE_BIT);
        await writeMemU32(this._stlink, FLASH_CR_REG, FLASH_CR_LOCK_BIT);

        // Step 7: System reset to reload option bytes
        await this.core_reset();

        this._dbg.warning("RDP Level 1 programmed. F1 requires a POWER CYCLE for RDP to take effect.");
    }

    /** Wait for BSY to clear without throwing on errors (for erase ops where PGERR is not meaningful). */
    async _wait_flash_busy_no_throw(timeout_sec) {
        const end_time = Date.now() + (timeout_sec * 2 * 1000);
        while (Date.now() < end_time) {
            let sr = await readMemU32(this._stlink, FLASH_SR_REG);
            if (!(sr & FLASH_SR_BUSY_BIT)) {
                return;
            }
            await async_sleep(timeout_sec / 20);
        }
        throw new Exception("Timeout waiting for flash operation to complete");
    }
}

// support STM32F MCUs with page access to FLASH and two banks
// (STM32F1xxxF and STM32F1xxxG) (XL devices)
const STM32FPXL_BANK_SIZE = 512 * 1024;
class Stm32FPXL extends Stm32FP {
    async flash_erase_all() {
        this._dbg.debug("Stm32F1.flash_erase_all()");
        await this._flash_erase_all(0);
        await this._flash_erase_all(1);
    }

    async flash_write(addr, data, { erase = false, verify = false, erase_sizes = null }) {
        let options = arguments[2];
        let addr_str = (addr !== null) ? `0x${H32(addr)}` : 'None';
        this._dbg.debug(`Stm32F1.flash_write(${addr_str}, [data:${data.length}Bytes], erase=${erase}, verify=${verify}, erase_sizes=${erase_sizes})`);
        var addr_bank1, addr_bank2, data_bank1, data_bank2;
        if (addr === null) {
            addr = this.FLASH_START;
        } else {
            if (addr % 2) {
                throw new Exception("Start address is not aligned to half-word");
            }
        }
        if ((addr - this.FLASH_START + data.length) <= STM32FPXL_BANK_SIZE) {
            await this._flash_write(addr, data, { ...options, bank: 0 });
        } else {
            if ((addr - this.FLASH_START) > STM32FPXL_BANK_SIZE) {
                await this._flash_write(addr, data, { ...options, bank: 1 });
            } else {
                addr_bank1 = addr;
                addr_bank2 = (this.FLASH_START + STM32FPXL_BANK_SIZE);
                data_bank1 = data.slice(0, (STM32FPXL_BANK_SIZE - (addr - this.FLASH_START)));
                data_bank2 = data.slice(STM32FPXL_BANK_SIZE - (addr - this.FLASH_START));
                await this._flash_write(addr_bank1, data_bank1, { ...options, bank: 0 });
                await this._flash_write(addr_bank2, data_bank2, { ...options, bank: 1 });
            }
        }
    }
}

export { Stm32FP, Stm32FPXL };
