/**
 * TypeScript declarations for vendored webstlink library.
 * Source: https://github.com/devanlai/webstlink
 */

/** USB device filters for navigator.usb.requestDevice() */
export interface UsbDeviceFilter {
  vendorId: number;
  productId: number;
}

/** CPU detection result from detect_cpu() */
export interface CpuInfo {
  part_no: number;
  core: string;
  dev_id: number;
  type: string;
  flash_size: number;
  sram_size: number;
  flash_start: number;
  sram_start: number;
  eeprom_size: number;
  freq: number;
}

/** Logger class for webstlink debug output */
export declare class Logger {
  constructor(verbosity?: number);
  debug(msg: string): void;
  verbose(msg: string): void;
  info(msg: string): void;
  warning(msg: string): void;
  error(msg: string): void;
}

/** Main WebStlink class — high-level API for ST-LINK debugger */
export default class WebStlink {
  constructor(dbg?: Logger | null);

  /** Whether the ST-LINK is currently connected */
  readonly connected: boolean;

  /** Firmware compatibility warning (null if firmware is recent enough) */
  readonly firmware_warning: string | null;

  /** ST-LINK version info (available after attach, null before) */
  readonly version_info: {
    stlink: number;
    jtag: number;
    swim: number | null;
    mass: number | null;
    api: number;
    str: string;
    firmware_warning: string | null;
  } | null;

  /** Attach to a WebUSB device */
  attach(device: USBDevice, deviceDbg?: Logger | null): Promise<void>;

  /** Detach from the device */
  detach(): Promise<void>;

  /** Detect the connected CPU. Pass [] to auto-detect. */
  detect_cpu(
    expectedCpus: string[],
    pickCpu?: ((mcus: string[]) => Promise<string>) | null
  ): Promise<CpuInfo>;

  /** Read memory from target */
  read_memory(addr: number, size: number): Promise<Uint8Array>;

  /** Flash firmware to target (handles erase + write + internal verify) */
  flash(addr: number, data: Uint8Array | ArrayBuffer): Promise<void>;

  /** Reset the target CPU. halt=false → run immediately. */
  reset(halt: boolean): Promise<void>;

  /** Halt the target CPU */
  halt(): Promise<void>;

  /** Resume the target CPU */
  run(): Promise<void>;

  /** Read flash protection status. Returns null if driver doesn't support it. */
  read_flash_protection(): Promise<FlashProtectionInfo | null>;

  /** Remove all flash protection (erase + reprogram option bytes). Triggers system reset. */
  remove_flash_protection(): Promise<void>;

  /** Enable flash read protection (RDP Level 1). Triggers system reset. */
  enable_flash_protection(): Promise<void>;

  /** Re-enter SWD debug mode after target system reset. */
  reinit_debug(): Promise<void>;
}

/** F1-style protection (per-page bitmask) — STM32F0xx/F1xx/F3xx */
export interface FlashProtectionInfoF1 {
  type: "f1";
  /** True if RDP Level 1 (read protection) is active */
  rdp_active: boolean;
  /** Raw FLASH_WRPR register value. Bit=0 means protected, bit=1 means unprotected. */
  wrpr_raw: number;
}

/** L4-style protection (area ranges) — STM32L4xx */
export interface FlashProtectionInfoL4 {
  type: "l4";
  /** True if RDP is active (OPTR[7:0] !== 0xAA) */
  rdp_active: boolean;
  /** Raw RDP byte from OPTR[7:0] */
  rdp_level: number;
  /** WRP Area A start page (WRP1AR[7:0]) */
  wrp1a_start: number;
  /** WRP Area A end page (WRP1AR[23:16]) */
  wrp1a_end: number;
  /** WRP Area B start page (WRP1BR[7:0]) */
  wrp1b_start: number;
  /** WRP Area B end page (WRP1BR[23:16]) */
  wrp1b_end: number;
  /** Raw OPTR register value */
  optr_raw: number;
}

/** Flash protection status — discriminated union for F1 and L4 formats */
export type FlashProtectionInfo = FlashProtectionInfoF1 | FlashProtectionInfoL4;

/** Library namespace re-exported from lib/package.js */
export declare const usb: {
  Connector: new (dev: USBDevice, dbg?: Logger | null) => unknown;
  filters: UsbDeviceFilter[];
};
