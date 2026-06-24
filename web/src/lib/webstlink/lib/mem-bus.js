/**
 * 32-bit load/store via ST-Link READMEM/WRITEMEM (AHB / system bus).
 *
 * ST-Link READDEBUGREG/WRITEDEBUGREG target the CoreSight DAP register file only;
 * they do not reliably access peripheral registers (0x4002xxxx) or option bytes (0x1ffff8xx).
 *
 * NOTE: JavaScript bitwise operators work on signed 32-bit integers. Addresses above
 * 0x7FFFFFFF (like 0xE000xxxx) must use >>> 0 after bitwise ops to stay unsigned.
 */

import { Exception } from './stlinkex.js';

/**
 * @param {import('./stlinkv2.js').default} stlink
 * @param {number} addr
 * @returns {Promise<number>}
 */
export async function readMemU32(stlink, addr) {
    const aligned = (addr & ~3) >>> 0;
    const rx = await stlink.get_mem32(aligned, 4);
    const view =
        rx instanceof DataView
            ? rx
            : new DataView(rx.buffer, rx.byteOffset, rx.byteLength);
    const off = (addr - aligned) | 0;
    if (view.byteLength < off + 4) {
        throw new Exception(
            `readMemU32: short read at 0x${(addr >>> 0).toString(16)} (got ${view.byteLength} bytes)`
        );
    }
    return view.getUint32(off, true) >>> 0;
}

/**
 * @param {import('./stlinkv2.js').default} stlink
 * @param {number} addr
 * @param {number} value
 * @returns {Promise<void>}
 */
export async function writeMemU32(stlink, addr, value) {
    const buf = new ArrayBuffer(4);
    new DataView(buf).setUint32(0, value >>> 0, true);
    await stlink.set_mem32((addr & ~3) >>> 0, new Uint8Array(buf));
}

/**
 * 16-bit store for option byte / flash programming.
 *
 * Writes a 32-bit word where the target halfword is set to `value` and the other
 * halfword is 0xFFFF (erased/no-op). This avoids a read-modify-write cycle that
 * can corrupt the write when the option byte area is unreadable (e.g. under RDP).
 *
 * @param {import('./stlinkv2.js').default} stlink
 * @param {number} addr halfword-aligned (even)
 * @param {number} value 16-bit value
 */
export async function writeMemU16(stlink, addr, value) {
    if (addr & 1) {
        throw new Error('writeMemU16: address must be halfword-aligned');
    }
    const aligned = (addr & ~3) >>> 0;
    const shift = addr & 2 ? 16 : 0;
    // Place value in the target halfword, 0xFFFF in the other (erased = no-op for FPEC)
    const word = (0xFFFFFFFF & ~(0xFFFF << shift) | ((value & 0xFFFF) << shift)) >>> 0;
    await writeMemU32(stlink, aligned, word);
}
