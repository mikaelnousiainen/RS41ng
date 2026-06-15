/* webstlink.js
 * STM32 / ST-Link debugger front-end
 *
 * Copyright Devan Lai 2017
 *
 * Ported from pystlink.py in the pystlink project,
 * Copyright Pavel Revak 2015
 *
 */

import * as libstlink from './lib/package.js';
import Mutex from './mutex.js';
import {
    hex_word as H32,
    hex_string
} from './lib/util.js';


const CPUID_REG = 0xe000ed00;

function H24(v) {
    return hex_string(v, 3);
}

const TARGET_UNKNOWN = "UNKNOWN";
const TARGET_RUNNING = "RUNNING";
const TARGET_HALTED = "HALTED";
const TARGET_RESET = "RESET";
const TARGET_DEBUG_RUNNING = "DEBUG_RUNNING";

class TargetCache {
    constructor(webstlink) {
        this._webstlink = webstlink;
        this.state = TARGET_UNKNOWN;
        this.registers = {};
        this.num_registers = undefined;
    }

    update_state(status, flush) {
        const prev_state = this.state;
        if (status.halted) {
            this.state = TARGET_HALTED;
            if ((prev_state != TARGET_HALTED && prev_state != TARGET_RESET) || flush) {
                this.invalidate_registers();
                this._webstlink._dispatch_callback('halted', status);
            }
        } else {
            if (prev_state != TARGET_DEBUG_RUNNING) {
                this.state = TARGET_RUNNING;
                if ((prev_state != TARGET_RUNNING) || flush) {
                    this.invalidate_registers();
                    this._webstlink._dispatch_callback('resumed', status);
                }
            }
        }
    }

    invalidate_registers() {
        this.registers = {};
    }

    get_register(name) {
        return this.registers[name];
    }

    set_register(name, value) {
        this.registers[name] = value;
    }

    set_num_registers(count) {
        this.num_registers = count;
    }

    has_all_registers() {
        return (this.num_registers === Object.entries(this.registers).length);
    }
}

export default class WebStlink {
    constructor(dbg = null) {
        this._stlink = null;
        this._driver = null;
        this._dbg = dbg;
        this._mcu = null;
        this._mutex = new Mutex;
        this._callbacks = {
            inspect: [],
            halted: [],
            resumed: [],
        };
        this._callback_mutex = new Mutex;
        this._cache = null;
    }

    add_callback(name, handler) {
        if (this._callbacks[name] === undefined) {
            throw new Error(`No callback event type named ${name}`);
        }

        if (!(handler instanceof Function)) {
            throw new Error("Callback handler must be callable");
        }

        this._callbacks[name].push(handler);
    }

    _dispatch_callback(name, ...args) {
        let mutex = this._callback_mutex;
        async function run_callbacks(callbacks) {
            // Ensure that the previous callback chain is done before
            // starting this callback chain
            await mutex.lock();

            try {
                // Run callbacks in order
                for (let callback of callbacks) {
                    let result = callback.apply(undefined, args);
                    if (result instanceof Promise) {
                        result = await result;
                    }

                    // Stop callbacks if done
                    if (result === false) {
                        break;
                    }
                }
            } catch (err) {
                throw new libstlink.exceptions.Exception("Error while executing callback for " + name + ":" + err);
            } finally {
                mutex.unlock();
            }
        }

        // Run callbacks without waiting to avoid deadlock
        run_callbacks(this._callbacks[name].slice(0));
    }

    async attach(device, device_dbg = null) {
        await this._mutex.lock();
        try {
            let connector = new libstlink.usb.Connector(device, device_dbg);
            let stlink = new libstlink.Stlinkv2(connector, this._dbg);
            try {
                await connector.connect();
                try {
                    await stlink.init();
                } catch (e) {
                    try {
                        await stlink.clean_exit();
                    } catch (exit_err) {
                        if (this._dbg) {
                            this._dbg.warning("Error while attempting to exit cleanly: " + exit_err);
                        }
                    } finally {
                        throw e;
                    }
                }
            } catch (e) {
                try {
                    await connector.disconnect();
                } catch (disconnect_err) {
                    if (this._dbg) {
                        this._dbg.warning("Error while attempting to disconnect: " + disconnect_err);
                    }
                } finally {
                    throw e;
                }
            }

            this._stlink = stlink;
            if (this._dbg) {
                this._dbg.info("DEVICE: ST-Link/" + this._stlink.ver_str);
            }
        } finally {
            this._mutex.unlock();
        }
    }

    async detach() {
        this._mutex.lock();
        try {
            if (this._stlink !== null) {
                try {
                    await this._stlink.clean_exit();
                } catch (exit_err) {
                    if (this._dbg) {
                        this._dbg.warning("Error while attempting to exit cleanly: " + exit_err);
                    }
                }

                if (this._stlink._connector !== null) {
                    try {
                        await this._stlink._connector.disconnect();
                    } catch (disconnect_err) {
                        if (this._dbg) {
                            this._dbg.warning("Error while attempting to disconnect: " + disconnect_err);
                        }
                    }
                }

                this._stlink = null;
            }
        } finally {
            this._mutex.unlock();
        }
    }

    get connected() {
        if (this._stlink !== null) {
            if (this._stlink._connector !== null) {
                // TODO:
                return true;
            }
        }
        return false;
    }

    get examined() {
        return (this._mcu !== null);
    }

    get firmware_warning() {
        return this._stlink ? this._stlink.firmware_warning : null;
    }

    get version_info() {
        if (!this._stlink) return null;
        return {
            stlink: this._stlink.ver_stlink,
            jtag: this._stlink.ver_jtag,
            swim: this._stlink.ver_swim,
            mass: this._stlink.ver_mass,
            api: this._stlink.ver_api,
            str: this._stlink.ver_str,
            firmware_warning: this._stlink.firmware_warning,
        };
    }

    async find_mcus_by_core() {
        let cpuid = await this._stlink.get_debugreg32(CPUID_REG);
        if (cpuid == 0) {
            throw new libstlink.exceptions.Exception("Not connected to CPU");
        }
        this._dbg.verbose("CPUID:  " + H32(cpuid));
        let partno = (0xfff & (cpuid >> 4));
        let mcus = libstlink.DEVICES.find(mcus => (mcus.part_no == partno));
        if (mcus) {
            this._mcus_by_core = mcus;
            return;
        }
        
        throw new libstlink.exceptions.Exception(`PART_NO: 0x${H24(partno)} is not supported`);
    }

    async find_mcus_by_devid() {
        let idcode = await this._stlink.get_debugreg32(this._mcus_by_core.idcode_reg);
        this._dbg.verbose("IDCODE: " + H32(idcode));
        let devid = (0xfff & idcode);
        let mcus = this._mcus_by_core.devices.find(mcus => (mcus.dev_id == devid));
        if (mcus) {
            this._mcus_by_devid = mcus;
            return;
        }
        throw new libstlink.exceptions.Exception(`DEV_ID: 0x${H24(devid)} is not supported`);
    }

    async find_mcus_by_flash_size() {
        const reg = this._mcus_by_devid.flash_size_reg;
        // Flash size is stored in system memory (e.g. 0x1FFFF7E0 on STM32F1), not in the
        // ARM CoreSight DAP register file. get_debugreg* reads DAP and often returns 0 here.
        //
        // Use 32-bit aligned READMEM like pystlink: get_mem8(2) on WebUSB returned byte pairs
        // in an order that decoded to impossible sizes (e.g. 0x00,0x20 → 8192KB vs 32KB as 0x20,0x00).
        const aligned = reg & ~3;
        const rx = await this._stlink.get_mem32(aligned, 4);
        const view =
            rx instanceof DataView
                ? rx
                : new DataView(rx.buffer, rx.byteOffset, rx.byteLength);
        const off = reg - aligned;
        let kb = view.getUint16(off, true);
        const validSizes = new Set(this._mcus_by_devid.devices.map((d) => d.flash_size));
        // If LE halfword is not a listed density, try BE halfword or low byte only (RDP/noise).
        if (!validSizes.has(kb)) {
            const be = view.getUint16(off, false);
            if (validSizes.has(be)) {
                kb = be;
            } else {
                const lo = view.getUint8(off);
                if (validSizes.has(lo)) {
                    kb = lo;
                }
            }
        }
        this._flash_size = kb;

        this._mcus = this._mcus_by_devid.devices.filter(
            mcu => (mcu.flash_size == this._flash_size)
        );
        if (this._mcus.length == 0) {
            throw new libstlink.exceptions.Exception(`Connected CPU with DEV_ID: 0x${H24(this._mcus_by_devid.dev_id)} and FLASH size: ${this._flash_size}KB is not supported`);
        }
    }

    fix_cpu_type(cpu_type) {
        cpu_type = cpu_type.toUpperCase();
        // now support only STM32
        if (cpu_type.startsWith("STM32")) {
            // change character on 10 position to 'x' where is package size code
            if (cpu_type.length > 9) {
                return cpu_type.substring(0, 10) + "x" + cpu_type.substring(11);
            }
            return cpu_type;
        }
        throw new libstlink.exceptions.Exception(`"${cpu_type}" is not STM32 family`);
    }

    filter_detected_cpu(expected_cpus) {
        let cpus = [];
        for (let detected_cpu of this._mcus) {
            for (let expected_cpu of expected_cpus) {
                expected_cpu = this.fix_cpu_type(expected_cpu);
                if (detected_cpu.type.startsWith(expected_cpu)) {
                    cpus.append(detected_cpu);
                    break;
                }
            }
        }

        if (cpus.length == 0) {
            let expected = expected_cpus.join(",");
            let possibilities = this._mcus.map(cpu => cpu.type).join(",");
            if (this._mcus.length > 1) {
                throw new libstlink.exceptions.Exception(`Connected CPU is not ${expected} but detected is one of ${possibilities}`);
            } else {
                throw new libstlink.exceptions.Exception(`Connected CPU is not ${expected} but detected is ${possibilities}`);
            }
        }
        this._mcus = cpus;
    }

    async find_sram_eeprom_size(pick_cpu = null) {
        // if is found more MCUS, then SRAM and EEPROM size
        // will be used the smallest of all (worst case)
        let sram_sizes = this._mcus.map(mcu => mcu.sram_size);
        let eeprom_sizes = this._mcus.map(mcu => mcu.eeprom_size);
        this._sram_size = Math.min.apply(null, sram_sizes);
        this._eeprom_size = Math.min.apply(null, eeprom_sizes);
        if (this._mcus.length > 1) {
            let diff = false;
            if (this._sram_size != Math.max.apply(null, sram_sizes)) {
                diff = true;
                if (pick_cpu === null) {
                    this._dbg.warning("Detected CPU family has multiple SRAM sizes");
                }
            }
            if (this._eeprom_size != Math.max.apply(null, eeprom_sizes)) {
                diff = true;
                if (pick_cpu === null) {
                    this._dbg.warning("Detected CPU family has multiple EEPROM sizes.");
                }
            }
            if (diff) {
                let mcu = null;
                if (pick_cpu) {
                    let type = await pick_cpu(this._mcus);
                    mcu = this._mcus.find(m => (m.type == type));
                }

                if (mcu) {
                    this._mcu = mcu;
                    this._sram_size = mcu.sram_size;
                    this._eeprom_size = mcu.eeprom_size;
                } else {
                    this._dbg.warning("Automatically choosing the MCU variant with the smallest flash and eeprom");
                    this._mcu = this._mcus.find(m => (m.sram_size == this._sram_size));
                }
            } else {
                this._mcu = this._mcus[0];
            }
        } else {
            this._mcu = this._mcus[0];
        }

        this._dbg.info(`SRAM:   ${this._sram_size}KB`);
        if (this._eeprom_size > 0) {
            this._dbg.info(`EEPROM: ${this._eeprom_size}KB`);
        }
    }

    load_driver() {
        let flash_driver = this._mcus_by_devid.flash_driver;
        if (flash_driver == "STM32FP") {
            this._driver = new libstlink.drivers.Stm32FP(this._stlink, this._dbg);
        } else if (flash_driver == "STM32FPXL") {
            this._driver = new libstlink.drivers.Stm32FPXL(this._stlink, this._dbg);
        } else if (flash_driver == "STM32FS") {
            this._driver = new libstlink.drivers.Stm32FS(this._stlink, this._dbg);
        } else if (flash_driver == "STM32L4") {
            this._driver = new libstlink.drivers.Stm32L4(this._stlink, this._dbg);
        } else {
            this._driver = new libstlink.drivers.Stm32(this._stlink, this._dbg);
        }
    }

    async detect_cpu(expected_cpus, pick_cpu = null) {
        this._dbg.info(`SUPPLY: ${this._stlink.target_voltage.toFixed(2)}V`);
        this._dbg.verbose("COREID: " + H32(this._stlink.coreid));
        if (this._stlink.coreid == 0) {
            throw new libstlink.exceptions.Exception("Not connected to CPU");
        }
        await this._mutex.lock();
        try {
            await this.find_mcus_by_core();
            this._dbg.info("CORE:   " + this._mcus_by_core.core);
            await this.find_mcus_by_devid();
            await this.find_mcus_by_flash_size();
            if (expected_cpus.count > 0) {
                // filter detected MCUs by selected MCU type
                this.filter_detected_cpu(expected_cpus);
            }
            this._dbg.info("MCU:    " + this._mcus.map(mcu => mcu.type).join("/"));
            this._dbg.info(`FLASH:  ${this._flash_size}KB`);
            await this.find_sram_eeprom_size(pick_cpu);
            this.load_driver();
            this._last_cpu_status = null;

            const info = {
                part_no: this._mcus_by_core.part_no,
                core: this._mcus_by_core.core,
                dev_id: this._mcus_by_devid.dev_id,
                type: this._mcu.type,
                flash_size: this._flash_size,
                sram_size: this._sram_size,
                flash_start: this._driver.FLASH_START,
                sram_start: this._driver.SRAM_START,
                eeprom_size: this._eeprom_size,
                freq: this._mcu.freq,
            };

            this._cache = new TargetCache(this);

            return info;
        } finally {
            this._mutex.unlock();
        }
    }

    async _unsafe_inspect_cpu(flush = false) {
        let dhcsr = await this._driver.core_status();
        let lockup = (dhcsr & libstlink.drivers.Stm32.DHCSR_STATUS_LOCKUP_BIT) != 0;
        if (lockup) {
            this._dbg.verbose("Clearing lockup");
            await this._driver.core_halt();
            dhcsr = await this._driver.core_status();
        }
        let status = {
            halted: (dhcsr & libstlink.drivers.Stm32.DHCSR_STATUS_HALT_BIT) != 0,
            debug:  (dhcsr & libstlink.drivers.Stm32.DHCSR_DEBUGEN_BIT) != 0,
        };

        this._last_cpu_status = status;
        this._dispatch_callback('inspect', status);
        this._cache.update_state(status, flush);

        return status;
    }

    async inspect_cpu() {
        await this._mutex.lock();
        try {
            return await this._unsafe_inspect_cpu();
        } finally {
            this._mutex.unlock();
        }
    }

    get last_cpu_status() {
        return this._last_cpu_status;
    }

    async set_debug_enable(enabled) {
        await this._mutex.lock();
        try {
            if (enabled) {
                await this._driver.core_run();
            } else {
                await this._driver.core_nodebug();
            }
            await this._unsafe_inspect_cpu();
        } finally {
            this._mutex.unlock();
        }
    }

    async step() {
        await this._mutex.lock();
        try {
            await this._driver.core_step();
            await this._unsafe_inspect_cpu(true);
        } finally {
            this._mutex.unlock();
        }
    }

    async halt() {
        await this._mutex.lock();
        try {
            await this._driver.core_halt();
            await this._unsafe_inspect_cpu();
        } finally {
            this._mutex.unlock();
        }
    }

    async run() {
        await this._mutex.lock();
        try {
            await this._driver.core_run();
            await this._unsafe_inspect_cpu();
        } finally {
            this._mutex.unlock();
        }
    }

    async reset(halt) {
        await this._mutex.lock();
        try {
            if (halt) {
                await this._driver.core_reset_halt();
            } else {
                await this._driver.core_reset();
                // Flash programming ends with core_reset_halt in driver lock();
                // Explicitly release halt so firmware runs (vector fetch + main).
                await this._driver.core_run();
            }
            await this._unsafe_inspect_cpu(true);
        } finally {
            this._mutex.unlock();
        }
    }

    async read_registers() {
        await this._mutex.lock();
        try {
            return await this._unsafe_read_registers();
        } finally {
            this._mutex.unlock();
        }
    }

    async read_register(name) {
        await this._mutex.lock();
        try {
            return await this._unsafe_read_register(name);
        } finally {
            this._mutex.unlock();
        }
    }

    async read_instruction(pc) {
        await this._mutex.lock();
        try {
            if (pc === undefined) {
                pc = await this._unsafe_read_register("PC");
            }

            const addr = pc & 0xfffffffe;
            return await this._stlink.get_debugreg16(addr);
        } finally {
            this._mutex.unlock();
        }
    }

    async read_memory(addr, size) {
        await this._mutex.lock();
        try {
            return await this._driver.get_mem(addr, size);
        } finally {
            this._mutex.unlock();
        }
    }

    async flash(addr, data) {
        if (data instanceof ArrayBuffer) {
            data = new Uint8Array(data);
        } else if (data instanceof DataView) {
            data = new Uint8Array(data.buffer);
        } else if (data instanceof Array) {
            let new_data = new Uint8Array(data.length);
            for (let i=0; i < data.length; i++) {
                if (typeof data[i] != "number" || data[i] < 0x00 || data[i] > 0xff) {
                    throw new libstlink.exceptions.Exception(`Datum at index ${i} is not a valid octet: ${data[i]}`);
                }
            }
            data = new_data;
        } else if (!(data instanceof Uint8Array)) {
            throw new libstlink.exceptions.Exception(`Data of type ${typeof data} is not supported`);
        }
        await this._mutex.lock();
        try {
            await this._driver.flash_write(addr, data, {
                erase: true,
                verify: true,
                erase_sizes: this._mcus_by_devid.erase_sizes
            });
            this._cache.invalidate_registers();
            await this._unsafe_inspect_cpu(true);
        } finally {
            this._mutex.unlock();
        }
    }

    /**
     * Read flash protection status (RDP + per-page write protection).
     * Returns null if the flash driver doesn't support protection reading.
     */
    async read_flash_protection() {
        await this._mutex.lock();
        try {
            if (!this._driver || typeof this._driver.read_flash_protection !== 'function') {
                return null;
            }
            return await this._driver.read_flash_protection(this._mcu?.type ?? '');
        } finally {
            this._mutex.unlock();
        }
    }

    /**
     * Remove all flash protection by erasing and reprogramming option bytes.
     * Triggers a system reset to reload option bytes.
     */
    async remove_flash_protection() {
        await this._mutex.lock();
        try {
            if (!this._driver || typeof this._driver.remove_flash_protection !== 'function') {
                throw new libstlink.exceptions.Exception(
                    "Flash protection removal not supported for this MCU"
                );
            }
            await this._driver.remove_flash_protection(this._mcu?.type ?? '');
        } finally {
            this._mutex.unlock();
        }
    }

    /**
     * Enable flash read protection (RDP Level 1) by programming option bytes.
     * Triggers a system reset to reload option bytes.
     */
    async enable_flash_protection() {
        await this._mutex.lock();
        try {
            if (!this._driver || typeof this._driver.enable_flash_protection !== 'function') {
                throw new libstlink.exceptions.Exception(
                    "Flash protection enable not supported for this MCU"
                );
            }
            await this._driver.enable_flash_protection(this._mcu?.type ?? '');
        } finally {
            this._mutex.unlock();
        }
    }

    /**
     * Re-enter SWD debug mode after a target system reset.
     * The ST-LINK USB connection survives target resets; only the
     * debug session is lost and needs re-establishment.
     */
    async reinit_debug() {
        await this._mutex.lock();
        try {
            await this._stlink.enter_debug_swd();
            await this._stlink.read_coreid();
        } finally {
            this._mutex.unlock();
        }
    }

    async _unsafe_read_register(name) {
        let value = this._cache.get_register(name);
        if (value !== undefined) {
            return value;
        }
        value = await this._driver.get_reg(name);
        this._cache.set_register(name, value);

        return value;
    }

    async _unsafe_read_registers() {
        if (this._cache.has_all_registers()) {
            return this._cache.registers;
        } else {
            let registers = await this._driver.get_reg_all();
            this._cache.set_num_registers(Object.entries(registers).length);
            this._cache.registers = registers;
            return registers;
        }
    }

    async _unsafe_set_register(name, value) {
        await this._driver.set_reg(name, value);
        this._cache.set_register(name, value);
    }

    async _unsafe_read_semihost_params(pointer, count) {
        let param_bytes = await this._driver.get_mem(pointer, count * 4);
        let view = new DataView(param_bytes.buffer);
        let params = [];
        for (let i=0; i < count; i++) {
            params.push(view.getUint32(i*4, true));
        }
        return params;
    }

    async _unsafe_read_semihost_operation() {
        let opcode = await this._unsafe_read_register("R0");
        let pointer = await this._unsafe_read_register("R1");
        let operation = { "opcode": opcode };
        const opcodes = libstlink.semihosting.opcodes;
        if (opcode == opcodes.SYS_OPEN) {
            let params = await this._unsafe_read_semihost_params(pointer, 3);
            let [filename_pointer, mode, filename_length] = params;
            const modes = ["r", "rb", "r+", "r+b", "w", "wb", "w+", "w+b", "a", "ab", "a+", "a+b"];
            let filename_bytes = await this._driver.get_mem(filename_pointer, filename_length);
            operation.mode = modes[mode];
            operation.filename = String.fromCharCode.apply(undefined, filename_bytes);
        } else if (opcode == opcodes.SYS_CLOSE) {
            let params = await this._unsafe_read_semihost_params(pointer, 1);
            [operation.handle] = params;
        } else if (opcode == opcodes.SYS_WRITE) {
            let params = await this._unsafe_read_semihost_params(pointer, 3);
            let [handle, data_pointer, data_length] = params;
            operation.handle = handle;
            operation.data = await this._driver.get_mem(data_pointer, data_length);
        } else if (opcode == opcodes.SYS_ISTTY) {
            let params = await this._unsafe_read_semihost_params(pointer, 1);
            [operation.handle] = params;
        } else if (opcode == opcodes.SYS_FLEN) {
            let params = await this._unsafe_read_semihost_params(pointer, 1);
            [operation.handle] = params;
        }

        return operation;
    }

    async handle_semihosting(syscall_handler) {
        await this._mutex.lock();
        try {
            // Check if we're halted on a semihost syscall
            let pc = await this._unsafe_read_register("PC");
            const addr = pc & 0xfffffffe;
            let inst = await this._stlink.get_debugreg16(addr);
            if (inst != 0xBEAB) {
                return false;
            }

            // Read the syscall params and pass them to the handler
            let oper = await this._unsafe_read_semihost_operation();
            let result = syscall_handler(oper);
            if (parseInt(result) != result) {
                throw new libstlink.exceptions.Exception("Non-numeric semihost syscall result " + result);
            }

            // Return the syscall result in R0 and resume
            await this._unsafe_set_register("R0", result);
            await this._unsafe_set_register("PC", pc + 2);
            await this._driver.core_run();
            await this._unsafe_inspect_cpu(true);
            return true;
        } finally {
            this._mutex.unlock();
        }
    }
}
