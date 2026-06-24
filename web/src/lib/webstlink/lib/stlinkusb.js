/* stlinkusb.js
 * Low-level ST-Link USB communication wrapper class
 *
 * Copyright Devan Lai 2017
 *
 * Ported from lib/stlinkusb.py in the pystlink project,
 * Copyright Pavel Revak 2015
 *
 */

import { Exception, UsbError } from './stlinkex.js';
import {
    hex_halfword as H16,
    hex_octet_array
} from './util.js';

const STLINK_CMD_SIZE_V2 = 16;
const DEV_TYPES = [
    // ST-LINK V2: original, clones, and variants
    { 'version': 'V2',   'idVendor': 0x0483, 'idProduct': 0x3748, 'outPipe': 0x02, 'inPipe': 0x81 },
    // ST-LINK V2-1: integrated on Nucleo/Discovery boards
    { 'version': 'V2-1', 'idVendor': 0x0483, 'idProduct': 0x374a, 'outPipe': 0x01, 'inPipe': 0x81 },
    { 'version': 'V2-1', 'idVendor': 0x0483, 'idProduct': 0x374b, 'outPipe': 0x01, 'inPipe': 0x81 },
    // ST-LINK V3: STLINK-V3SET, V3MINI, and variants
    { 'version': 'V3',   'idVendor': 0x0483, 'idProduct': 0x374e, 'outPipe': 0x01, 'inPipe': 0x81 },
    { 'version': 'V3',   'idVendor': 0x0483, 'idProduct': 0x374f, 'outPipe': 0x01, 'inPipe': 0x81 },
    { 'version': 'V3',   'idVendor': 0x0483, 'idProduct': 0x3753, 'outPipe': 0x01, 'inPipe': 0x81 },
];

var Connector = class StlinkUsbConnector {
    constructor(dev, dbg = null) {
        this._dbg = dbg;
        this._dev = null;
        this._dev_type = null;
        for (let dev_type of DEV_TYPES) {
            if (dev.vendorId == dev_type.idVendor && dev.productId == dev_type.idProduct) {
                this._dev = dev;
                this._dev_type = dev_type;
                return;
            }
        }

        throw new Exception(`Unknown ST-Link device ${H16(dev.vendorId)}:${H16(dev.productId)}`);
    }

    async connect() {
        await this._dev.open();
        if (this._dev.configuration != 1) {
            await this._dev.selectConfiguration(1);
        }
        let intf = this._dev.configuration.interfaces[0];
        if (!intf.claimed) {
            await this._dev.claimInterface(0);
        }
        if (intf.alternate === null || intf.alternate.alternateSetting != 0) {
            await this._dev.selectAlternateInterface(0, 0);
        }
    }

    async disconnect() {
        try {
            await this._dev.close();
        } catch (error) {
            if (this._dbg) {
                this._dbg.debug("Error when disconnecting: " + error);
            }
        }
    }

    get version() {
        return this._dev_type.version;
    }

    get xfer_counter() {
        return this._xfer_counter;
    }

    _debug(msg) {
        if (this._dbg) {
            this._dbg.debug(msg);
        }
    }

    async _write(data) {
        if (this._dbg) {
            const bytes = new Uint8Array(data);
            const hex_bytes = hex_octet_array(bytes);
            this._dbg.debug("  USB > " + hex_bytes.join(" "));
        }

        this._xfer_counter++;
        let result;
        try {
            result = await this._dev.transferOut(this._dev_type.outPipe, data);
            if (result.status != "ok") {
                throw result.status;
            }
        } catch (e) {
            if (e instanceof DOMException || e.constructor == String) {
                throw new UsbError(e, this._dev_type.outPipe);
            } else {
                throw e;
            }
        }

        if (result.bytesWritten != data.length) {
            throw new Exception(`Error, only ${result.bytesWritten} Bytes was transmitted to ST-Link instead of expected ${data.length}`);
        }
    }

    async _read(size) {
        let read_size = size;
        if (read_size < 64) {
            read_size = 64;
        } else if (read_size % 4) {
            read_size += 3;
            read_size &= 0xffc;
        }

        let result;
        try {
            result = await this._dev.transferIn(this._dev_type.inPipe & 0x7f, read_size);
            if (result.status != "ok") {
                throw result.status;
            }
        } catch (e) {
            if (e instanceof DOMException || e.constructor == String) {
                throw new UsbError(e, this._dev_type.inPipe);
            } else {
                throw e;
            }
        }

        if (this._dbg) {
            const bytes = new Uint8Array(result.data.buffer);
            const hex_bytes = hex_octet_array(bytes);
            this._dbg.debug("  USB < " + hex_bytes.join(" "));
        }

        if (result.data.byteLength > size) {
            return new DataView(result.data.buffer, result.data.byteOffset, size);
        } else {
            return result.data;
        }
    }

    async xfer(cmd, { data=null, rx_len=null, retry=0 } = {}) {
        let src;
        if (cmd instanceof Array || cmd instanceof Uint8Array) {
            src = cmd;
        } else if (cmd instanceof ArrayBuffer) {
            src = new Uint8Array(cmd);
        } else if (cmd instanceof DataView) {
            src = new Uint8Array(cmd.buffer);
        } else {
            throw new Exception(`Unsupported command datatype ${typeof cmd}`);
        }

        if (src.length > STLINK_CMD_SIZE_V2) {
            throw new Exception(`Error too many Bytes in command: ${src.length}, maximum is ${STLINK_CMD_SIZE_V2}`);
        }

        // pad to 16 bytes
        let cmd_buffer = new Uint8Array(STLINK_CMD_SIZE_V2);
        src.forEach((v, i) => cmd_buffer[i] = v);

        while (true) {
            try {
                await this._write(cmd_buffer);
                if (data) {
                    await this._write(data);
                }
                if (rx_len) {
                    let rx = await this._read(rx_len);
                    return rx;
                }
            } catch (e) {
                if ((e instanceof UsbError) && !e.fatal && (retry > 0)) {
                    this._debug("Retrying xfer after " + e);
                    retry--;
                    continue;
                } else {
                    throw e;
                }
            }
            return;
        }
    }
};

const filters = DEV_TYPES.map(
    dev_type => ({
        vendorId:  dev_type.idVendor,
        productId: dev_type.idProduct
    })
);

export { Connector, filters };
