/* stm32l4.js
 * STM32L4xx flash driver
 *
 * Supports STM32L4 series MCUs with uniform 2KB page flash and
 * 64-bit (double-word) write alignment.
 *
 * Covers: STM32L412, STM32L422, STM32L43x, STM32L44x, STM32L45x,
 *         STM32L46x, STM32L47x, STM32L48x, STM32L49x, STM32L4Ax
 *
 * Based on stm32fp.js architecture (inner Flash class + outer Stm32 subclass).
 */

import { Exception } from './stlinkex.js';
import { Stm32 } from './stm32.js';
import {
    hex_word as H32,
    async_sleep,
} from './util.js';
import { readMemU32, writeMemU32 } from './mem-bus.js';

/** Read memory for verify using only 8-bit reads (matches stm32fp strategy). */
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

// ── Flash register addresses (STM32L4xx) ──────────────────────────────
const FLASH_REG_BASE    = 0x40022000;
const FLASH_KEYR_REG    = FLASH_REG_BASE + 0x08;
const FLASH_OPTKEYR_REG = FLASH_REG_BASE + 0x0c;
const FLASH_SR_REG      = FLASH_REG_BASE + 0x10;
const FLASH_CR_REG      = FLASH_REG_BASE + 0x14;
const FLASH_OPTR_REG    = FLASH_REG_BASE + 0x20;
const FLASH_WRP1AR_REG  = FLASH_REG_BASE + 0x2c;
const FLASH_WRP1BR_REG  = FLASH_REG_BASE + 0x30;

// ── FLASH_CR bits ──────────────────────────────────────────────────────
const FLASH_CR_PG_BIT         = (1 << 0);
const FLASH_CR_PER_BIT        = (1 << 1);
const FLASH_CR_MER1_BIT       = (1 << 2);
const FLASH_CR_PNB_SHIFT      = 3;
const FLASH_CR_STRT_BIT       = (1 << 16);
const FLASH_CR_OPTSTRT_BIT    = (1 << 17);
const FLASH_CR_OBL_LAUNCH_BIT = 0x08000000;  // bit 27
const FLASH_CR_OPTLOCK_BIT    = 0x40000000;  // bit 30
const FLASH_CR_LOCK_BIT       = 0x80000000;  // bit 31

// ── FLASH_SR bits ──────────────────────────────────────────────────────
const FLASH_SR_EOP_BIT     = (1 << 0);
const FLASH_SR_OPERR_BIT   = (1 << 1);
const FLASH_SR_PROGERR_BIT = (1 << 3);
const FLASH_SR_WRPERR_BIT  = (1 << 4);
const FLASH_SR_PGAERR_BIT  = (1 << 5);
const FLASH_SR_SIZERR_BIT  = (1 << 6);
const FLASH_SR_PGSERR_BIT  = (1 << 7);
const FLASH_SR_BSY_BIT     = (1 << 16);

const FLASH_SR_ERROR_MASK = (
    FLASH_SR_OPERR_BIT | FLASH_SR_PROGERR_BIT | FLASH_SR_WRPERR_BIT |
    FLASH_SR_PGAERR_BIT | FLASH_SR_SIZERR_BIT | FLASH_SR_PGSERR_BIT
);

// Flash unlock keys (same as F0/F1)
const FLASH_KEY1 = 0x45670123;
const FLASH_KEY2 = 0xcdef89ab;

// Option byte unlock keys (L4-specific — different from F0/F1!)
const FLASH_OPT_KEY1 = 0x08192a3b;
const FLASH_OPT_KEY2 = 0x4c5d6e7f;

// ── SRAM-resident flash writer (ARM Thumb, one 64-bit program per run) ───
//
// Two aligned str 32-bit to flash = one L4 double-word program. This payload
// performs exactly one program, waits for BSY clear, then bkpt.
// FlashL4.write() loops in JavaScript in 8-byte steps and calls clear_errors()
// between steps so EOP (and errors) are cleared via the debug port before the
// next program (RM requirement). A multi-iteration in-RAM loop + in-loop EOP
// clear failed on real silicon: first DW programmed, second stayed 0xFF.
//
// Register convention each run:
//   R0 = 8-byte source in SRAM, R1 = dest in flash (64-bit aligned)
//   R4 = FLASH_SR, R5 = BSY mask (0x00010000)
//
const FLASH_WRITER_L4_CODE = new Uint8Array([
    0x03, 0x68,  // 0x6803    // ldr  r3, [r0, #0]
    0x0b, 0x60,  // 0x600b    // str  r3, [r1, #0]
    0x43, 0x68,  // 0x6843    // ldr  r3, [r0, #4]
    0x4b, 0x60,  // 0x604b    // str  r3, [r1, #4]
    // test_busy:
    0x23, 0x68,  // 0x6823    // ldr  r3, [r4]
    0x2b, 0x42,  // 0x422b    // tst  r3, r5
    0xfc, 0xd1,  // 0xd1fc    // bne  test_busy
    0x00, 0xbe,  // 0xbe00    // bkpt #0
]);


// ── Inner FlashL4 class ─────────────────────────────────────────────────
// Low-level flash operations: unlock, erase, write algorithm, wait.

class FlashL4 {
    constructor(driver, stlink, dbg) {
        this._driver = driver;
        this._stlink = stlink;
        this._dbg = dbg;
    }

    async init() {
        await this._stlink.read_target_voltage();
        if (this._stlink.target_voltage < 1.71) {
            throw new Exception(
                `Supply voltage is ${this._stlink.target_voltage.toFixed(2)}V, ` +
                `but minimum for STM32L4 FLASH program/erase is 1.71V`
            );
        }
        await this.unlock();
        await this.clear_errors();
    }

    async unlock() {
        await this._driver.core_reset_halt();
        let cr = await readMemU32(this._stlink, FLASH_CR_REG);
        if (cr & FLASH_CR_LOCK_BIT) {
            await writeMemU32(this._stlink, FLASH_KEYR_REG, FLASH_KEY1);
            await writeMemU32(this._stlink, FLASH_KEYR_REG, FLASH_KEY2);
        }
        cr = await readMemU32(this._stlink, FLASH_CR_REG);
        if (cr & FLASH_CR_LOCK_BIT) {
            throw new Exception("Error unlocking FLASH");
        }
    }

    async lock() {
        await writeMemU32(this._stlink, FLASH_CR_REG, FLASH_CR_LOCK_BIT);
        await this._driver.core_reset_halt();
    }

    async clear_errors() {
        let sr = await readMemU32(this._stlink, FLASH_SR_REG);
        let clear_bits = sr & (FLASH_SR_ERROR_MASK | FLASH_SR_EOP_BIT);
        if (clear_bits) {
            await writeMemU32(this._stlink, FLASH_SR_REG, clear_bits);
        }
    }

    async erase_all() {
        await this.clear_errors();
        await writeMemU32(this._stlink, FLASH_CR_REG, FLASH_CR_MER1_BIT);
        await writeMemU32(
            this._stlink, FLASH_CR_REG, FLASH_CR_MER1_BIT | FLASH_CR_STRT_BIT
        );
        await this.wait_busy(2, "Erasing FLASH");
    }

    async erase_page(page_num) {
        await this.clear_errors();
        let cr_val = FLASH_CR_PER_BIT | (page_num << FLASH_CR_PNB_SHIFT);
        await writeMemU32(this._stlink, FLASH_CR_REG, cr_val);
        await writeMemU32(this._stlink, FLASH_CR_REG, cr_val | FLASH_CR_STRT_BIT);
        await this.wait_busy(0.2);
    }

    async erase_pages(flash_start, page_size, addr, size) {
        if (size === undefined) {
            throw new Exception("erase size is undefined");
        }
        let start_page = Math.floor((addr - flash_start) / page_size);
        let end_page = Math.ceil((addr + size - flash_start) / page_size) - 1;
        this._dbg.bargraph_start("Erasing FLASH", {
            "value_min": start_page,
            "value_max": end_page,
        });
        for (let page = start_page; page <= end_page; page++) {
            this._dbg.bargraph_update({"value": page});
            await this.erase_page(page);
        }
        this._dbg.bargraph_done();
    }

    async init_write(sram_offset) {
        this._dbg.debug("Loading L4 flash algorithm at 0x" + H32(sram_offset));
        this._flash_writer_offset = sram_offset;
        this._flash_data_offset = sram_offset + 256;
        await this._stlink.set_mem8(this._flash_writer_offset, FLASH_WRITER_L4_CODE);
        // Set register constants for the SRAM algorithm
        await this._driver.set_reg("R4", FLASH_SR_REG);
        await this._driver.set_reg("R5", FLASH_SR_BSY_BIT);
        // Enable programming mode
        await this.clear_errors();
        await writeMemU32(this._stlink, FLASH_CR_REG, FLASH_CR_PG_BIT);
    }

    async write(addr, block) {
        // Skip all-0xFF blocks (already erased)
        if (block.every(b => (b == 0xff))) {
            return;
        }

        this._dbg.debug(`Flashing ${block.length} bytes at 0x${H32(addr)}`);
        try {
            for (let off = 0; off < block.length; off += 8) {
                let chunk = block.subarray(off, off + 8);
                if (chunk.every(b => (b === 0xff))) {
                    continue;
                }
                await this.clear_errors();
                // L4 clears PG when a program operation completes; each new double-word needs PG set again.
                await writeMemU32(this._stlink, FLASH_CR_REG, FLASH_CR_PG_BIT);
                await this._stlink.set_mem32(this._flash_data_offset, chunk);
                await this._driver.set_reg("R0", this._flash_data_offset);
                await this._driver.set_reg("R1", addr + off);
                await this._driver.set_reg("PC", this._flash_writer_offset | 1);
                await this._driver.core_run();
                await this.wait_for_breakpoint(0.5);
            }
        } catch (e) {
            throw new Exception(
                `Failed to flash ${block.length} bytes at 0x${H32(addr)}: ` + e
            );
        }
    }

    async wait_busy(wait_time, bargraph_msg = null) {
        if (bargraph_msg) {
            this._dbg.bargraph_start(bargraph_msg, {
                "value_min": Date.now() / 1000.0,
                "value_max": (Date.now() / 1000.0 + wait_time),
            });
        }
        const end_time = Date.now() + (wait_time * 2 * 1000);
        while (Date.now() < end_time) {
            if (bargraph_msg) {
                this._dbg.bargraph_update({"value": Date.now() / 1000.0});
            }
            let status = await readMemU32(this._stlink, FLASH_SR_REG);
            if (!(status & FLASH_SR_BSY_BIT)) {
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
            let dhcsr = await readMemU32(this._stlink, Stm32.DHCSR_REG);
            this._dbg.debug("DHCSR = 0x" + H32(dhcsr));
            if (dhcsr & Stm32.DHCSR_STATUS_LOCKUP_BIT) {
                throw new Exception("Flash algorithm crashed");
            }
            if (dhcsr & Stm32.DHCSR_STATUS_HALT_BIT) {
                break;
            }
            await async_sleep(wait_time / 20);
        } while (Date.now() < end_time);

        let sr = await readMemU32(this._stlink, FLASH_SR_REG);
        await this.end_of_operation(sr);
    }

    async end_of_operation(status) {
        let errors = status & FLASH_SR_ERROR_MASK;
        if (errors) {
            // Clear error flags
            await writeMemU32(this._stlink, FLASH_SR_REG, errors);
            throw new Exception(
                "Error writing FLASH with status (FLASH_SR) " + H32(status)
            );
        }
        // Clear EOP
        if (status & FLASH_SR_EOP_BIT) {
            await writeMemU32(this._stlink, FLASH_SR_REG, FLASH_SR_EOP_BIT);
        }
    }
}


// ── Outer Stm32L4 class (extends Stm32 base) ───────────────────────────

class Stm32L4 extends Stm32 {

    async flash_erase_all() {
        this._dbg.debug("Stm32L4.flash_erase_all()");
        let flash = new FlashL4(this, this._stlink, this._dbg);
        await flash.init();
        await flash.erase_all();
        await flash.lock();
    }

    async flash_write(addr, data, { erase = false, verify = false, erase_sizes = null }) {
        let addr_str = (addr !== null) ? `0x${H32(addr)}` : 'None';
        this._dbg.debug(
            `Stm32L4.flash_write(${addr_str}, [data:${data.length}Bytes], ` +
            `erase=${erase}, verify=${verify}, erase_sizes=${erase_sizes})`
        );
        if (addr === null) {
            addr = this.FLASH_START;
        } else {
            if (addr % 8) {
                throw new Exception("Start address is not aligned to double-word (8 bytes)");
            }
        }

        // Pad data to 8-byte (double-word) alignment
        if (data.length % 8) {
            let padded = new Uint8Array(data.length + (8 - (data.length % 8)));
            padded.set(data);
            padded.fill(0xff, data.length);
            data = padded;
        }

        let flash = new FlashL4(this, this._stlink, this._dbg);
        await flash.init();

        if (erase) {
            if (erase_sizes) {
                let page_size = erase_sizes[0]; // uniform pages for L4
                await flash.erase_pages(this.FLASH_START, page_size, addr, data.length);
            } else {
                await flash.erase_all();
            }
        }

        this._dbg.bargraph_start("Writing FLASH", {
            "value_min": addr,
            "value_max": (addr + data.length),
        });
        await flash.init_write(Stm32L4.SRAM_START);

        // Block size must be a multiple of 8 for double-word writes
        let block_size = this._stlink.maximum_transfer_size & ~7;
        if (block_size < 8) block_size = 8;

        while (data.length > 0) {
            this._dbg.bargraph_update({"value": addr});
            let block = data.slice(0, block_size);
            data = data.slice(block_size);
            await flash.write(addr, block);
            if (verify) {
                this._dbg.debug(`Verifying ${block.length} bytes`);
                await writeMemU32(this._stlink, FLASH_CR_REG, 0);
                try {
                    let flashed_block = await readBytesMem8(this._stlink, addr, block.length);
                    let firstDiff = -1;
                    if (flashed_block.length == block.length) {
                        for (let i = 0; i < block.length; i++) {
                            if (flashed_block[i] !== block[i]) {
                                firstDiff = i;
                                break;
                            }
                        }
                    } else {
                        firstDiff = -2;
                    }
                    const ok = firstDiff < 0;
                    if (!ok) {
                        const encHex = (u8, n) => Array.from(u8.subarray(0, n)).map((x) => x.toString(16).padStart(2, "0")).join(" ");
                        const detail = firstDiff >= 0
                            ? ` firstMismatch@+${firstDiff} exp=0x${block[firstDiff].toString(16)} got=0x${flashed_block[firstDiff].toString(16)}`
                            : ` lenMismatch flashView=${flashed_block.length}`;
                        const fullMsg = "Verify error at block address: 0x" + H32(addr) + detail +
                            ` exp16=${encHex(block, 16)} rd16=${encHex(flashed_block, 16)}`;
                        this._dbg.error(`[L4 flash verify] blockLen=${block.length} ${fullMsg}`);
                        throw new Exception(fullMsg);
                    }
                } finally {
                    if (data.length > 0) {
                        await writeMemU32(this._stlink, FLASH_CR_REG, FLASH_CR_PG_BIT);
                    }
                }
            }
            addr += block.length;
        }

        await flash.lock();
        this._dbg.bargraph_done();
    }

    // ── Flash protection support ──────────────────────────────────────

    /**
     * Read flash protection status for STM32L4xx.
     *
     * Returns L4-specific protection info:
     *   - RDP level from OPTR register
     *   - WRP area ranges from WRP1AR / WRP1BR
     */
    async read_flash_protection(_cpuType) {
        this._dbg.debug("Stm32L4.read_flash_protection()");

        let optr = await readMemU32(this._stlink, FLASH_OPTR_REG);
        let wrp1ar = await readMemU32(this._stlink, FLASH_WRP1AR_REG);
        let wrp1br = await readMemU32(this._stlink, FLASH_WRP1BR_REG);

        let rdp_byte = optr & 0xff;

        return {
            type: "l4",
            rdp_active: (rdp_byte !== 0xaa),
            rdp_level: rdp_byte,
            wrp1a_start: wrp1ar & 0xff,
            wrp1a_end: (wrp1ar >> 16) & 0xff,
            wrp1b_start: wrp1br & 0xff,
            wrp1b_end: (wrp1br >> 16) & 0xff,
            optr_raw: optr,
        };
    }

    /**
     * Remove all flash protection by reprogramming option bytes.
     *
     * Sets RDP to Level 0 (0xAA), clears WRP area protection.
     * Uses OBL_LAUNCH to trigger option byte reload + system reset.
     *
     * WARNING: If RDP Level 1 was active, transitioning to Level 0
     * triggers a mass erase of the entire main flash.
     */
    async remove_flash_protection(_cpuType) {
        this._dbg.debug("Stm32L4.remove_flash_protection()");

        // Step 1: Halt the core
        await this.core_reset_halt();

        // Step 2: Unlock main flash (MEM-AP — not DAP debug-register file)
        let cr = await readMemU32(this._stlink, FLASH_CR_REG);
        if (cr & FLASH_CR_LOCK_BIT) {
            await writeMemU32(this._stlink, FLASH_KEYR_REG, FLASH_KEY1);
            await writeMemU32(this._stlink, FLASH_KEYR_REG, FLASH_KEY2);
        }
        cr = await readMemU32(this._stlink, FLASH_CR_REG);
        if (cr & FLASH_CR_LOCK_BIT) {
            throw new Exception("Failed to unlock flash for option byte access");
        }

        // Step 3: Unlock option bytes (L4-specific keys)
        if (cr & FLASH_CR_OPTLOCK_BIT) {
            await writeMemU32(this._stlink, FLASH_OPTKEYR_REG, FLASH_OPT_KEY1);
            await writeMemU32(this._stlink, FLASH_OPTKEYR_REG, FLASH_OPT_KEY2);
        }
        cr = await readMemU32(this._stlink, FLASH_CR_REG);
        if (cr & FLASH_CR_OPTLOCK_BIT) {
            throw new Exception("Failed to unlock option bytes (OPTLOCK still set)");
        }

        // Step 4: Read current OPTR and modify only the RDP byte (Level 0 = 0xAA)
        let optr = await readMemU32(this._stlink, FLASH_OPTR_REG);
        optr = (optr & 0xffffff00) | 0xaa;
        await writeMemU32(this._stlink, FLASH_OPTR_REG, optr);

        // Step 5: Clear WRP areas (start > end = no protection per RM)
        await writeMemU32(this._stlink, FLASH_WRP1AR_REG, 0x000000ff);
        await writeMemU32(this._stlink, FLASH_WRP1BR_REG, 0x000000ff);

        // Step 6: Start option byte programming — RMW FLASH_CR (do not wipe unlock / other bits)
        cr = await readMemU32(this._stlink, FLASH_CR_REG);
        await writeMemU32(this._stlink, FLASH_CR_REG, cr | FLASH_CR_OPTSTRT_BIT);
        await this._wait_flash_busy(2.0);

        // Step 7: Reload option bytes from flash — RMW OBL_LAUNCH per RM0351
        cr = await readMemU32(this._stlink, FLASH_CR_REG);
        await writeMemU32(this._stlink, FLASH_CR_REG, cr | FLASH_CR_OBL_LAUNCH_BIT);
    }

    /**
     * Enable flash read protection (RDP Level 1).
     * Modifies only the RDP field in OPTR to 0x00 (Level 1 for L4).
     *
     * SAFETY: Level 1 (RDP = 0x00) is reversible. Level 2 (RDP = 0xCC) is
     * permanent and irreversible — we NEVER write 0xCC.
     *
     * Going Level 0 → Level 1 does NOT trigger mass erase (safe).
     */
    async enable_flash_protection(_cpuType) {
        this._dbg.debug("Stm32L4.enable_flash_protection()");

        const RDP_LEVEL1 = 0x00;

        // SAFETY ASSERTION: Never write Level 2
        if (RDP_LEVEL1 === 0xcc || RDP_LEVEL1 === 0xCC) {
            throw new Exception("SAFETY: Refusing to write RDP Level 2 (0xCC) — this is irreversible!");
        }

        // Step 1: Halt the core
        await this.core_reset_halt();

        // Step 2: Unlock main flash
        let cr = await readMemU32(this._stlink, FLASH_CR_REG);
        if (cr & FLASH_CR_LOCK_BIT) {
            await writeMemU32(this._stlink, FLASH_KEYR_REG, FLASH_KEY1);
            await writeMemU32(this._stlink, FLASH_KEYR_REG, FLASH_KEY2);
        }
        cr = await readMemU32(this._stlink, FLASH_CR_REG);
        if (cr & FLASH_CR_LOCK_BIT) {
            throw new Exception("Failed to unlock flash for option byte access");
        }

        // Step 3: Unlock option bytes (L4-specific keys)
        if (cr & FLASH_CR_OPTLOCK_BIT) {
            await writeMemU32(this._stlink, FLASH_OPTKEYR_REG, FLASH_OPT_KEY1);
            await writeMemU32(this._stlink, FLASH_OPTKEYR_REG, FLASH_OPT_KEY2);
        }
        cr = await readMemU32(this._stlink, FLASH_CR_REG);
        if (cr & FLASH_CR_OPTLOCK_BIT) {
            throw new Exception("Failed to unlock option bytes (OPTLOCK still set)");
        }

        // Step 4: Read current OPTR and modify only the RDP byte to Level 1
        let optr = await readMemU32(this._stlink, FLASH_OPTR_REG);
        const newRdp = RDP_LEVEL1 & 0xff;

        // SAFETY: Final check before write — reject 0xCC
        if (newRdp === 0xcc) {
            throw new Exception("SAFETY: Refusing to write RDP Level 2 (0xCC) — this is irreversible!");
        }

        optr = (optr & 0xffffff00) | newRdp;
        await writeMemU32(this._stlink, FLASH_OPTR_REG, optr);

        // Step 5: Start option byte programming
        cr = await readMemU32(this._stlink, FLASH_CR_REG);
        await writeMemU32(this._stlink, FLASH_CR_REG, cr | FLASH_CR_OPTSTRT_BIT);
        await this._wait_flash_busy(2.0);

        // Step 6: Reload option bytes — OBL_LAUNCH triggers system reset
        cr = await readMemU32(this._stlink, FLASH_CR_REG);
        await writeMemU32(this._stlink, FLASH_CR_REG, cr | FLASH_CR_OBL_LAUNCH_BIT);
    }

    /**
     * Poll FLASH_SR until BSY clears. Checks for L4 error flags.
     */
    async _wait_flash_busy(timeout_sec) {
        const end_time = Date.now() + (timeout_sec * 2 * 1000);
        while (Date.now() < end_time) {
            let sr = await readMemU32(this._stlink, FLASH_SR_REG);
            if (!(sr & FLASH_SR_BSY_BIT)) {
                if (sr & FLASH_SR_ERROR_MASK) {
                    await writeMemU32(this._stlink, FLASH_SR_REG, sr & FLASH_SR_ERROR_MASK);
                    throw new Exception(
                        "Flash error during option byte operation (FLASH_SR) " + H32(sr)
                    );
                }
                if (sr & FLASH_SR_EOP_BIT) {
                    await writeMemU32(this._stlink, FLASH_SR_REG, FLASH_SR_EOP_BIT);
                }
                return;
            }
            await async_sleep(timeout_sec / 20);
        }
        throw new Exception("Timeout waiting for flash operation to complete");
    }
}

export { Stm32L4 };
