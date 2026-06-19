import type { Ref } from "vue";

export type FlashStatus =
  | "idle"
  | "connecting"
  | "connected"
  | "unlocking"
  | "erasing"
  | "flashing"
  | "verifying"
  | "done"
  | "error";

export interface FlashLogEntry {
  timestamp: number;
  level: "info" | "warn" | "error";
  message: string;
}

/** Structured identification of a connected ST-LINK programmer */
export interface StlinkDeviceInfo {
  // Device identification (from USB PID)
  model: string;
  family: "V2" | "V2-1" | "V3";
  formFactor: string;
  productId: number;

  // Firmware (from version read after attach)
  firmware: string;
  jtagVersion: number;
  apiVersion: number;

  // Derived capabilities
  swdFreqControl: boolean;
  highSpeedUsb: boolean;

  // Clone detection
  isLikelyClone: boolean;

  // Warnings
  firmwareWarning: string | null;
}

/** MCU family — determines flash algorithm and protection format */
export type McuFamily = "F1" | "L4";

/** Flash protection status (unified for F1 and L4) */
export interface FlashProtectionStatus {
  /** True if any write protection is active */
  writeProtected: boolean;
  /** True if read protection (RDP) is active */
  readProtected: boolean;
  /** Human-readable summary */
  summary: string;

  // F1-specific (optional)
  /** Number of protected 4KB page groups (F1 only) */
  protectedGroups?: number;
  /** Total number of valid page groups for this flash size (F1 only) */
  totalGroups?: number;
  /** Raw WRPR register value (F1 only) */
  wrprRaw?: number;

  // L4-specific (optional)
  /** WRP area ranges (L4 only) */
  wrpAreas?: { name: string; startPage: number; endPage: number }[];
  /** Raw RDP level byte (L4 only) */
  rdpLevel?: number;
  /** Raw OPTR register value (L4 only) */
  optrRaw?: number;
}

/** Narrow store interface that flash strategies operate on */
export interface FlasherStore {
  status: Ref<FlashStatus>;
  progress: Ref<number>;
  chipId: Ref<string | null>;
  deviceInfo: Ref<StlinkDeviceInfo | null>;
  protectionStatus: Ref<FlashProtectionStatus | null>;
  mcuFamily: Ref<McuFamily | null>;
  expectedMcuFamily: McuFamily | null;
  errorMessage: Ref<string | null>;
  binaryFile: Ref<File | null>;
  addLog(level: FlashLogEntry["level"], message: string): void;
}

/** Interface for pluggable flash transports */
export interface FlashStrategy {
  /** Unique strategy identifier */
  readonly id: string;
  /** Human-readable label */
  readonly label: string;
  /** Whether this strategy is available in the current browser */
  readonly available: boolean;

  /** Connect to the programmer hardware. Sets store status/chipId. */
  connect(store: FlasherStore): Promise<void>;
  /** Flash firmware binary. Sets store status/progress. */
  flash(store: FlasherStore, verify: boolean): Promise<void>;
  /** Disconnect from the programmer. */
  disconnect(): Promise<void>;

  /** Remove flash protection (optional — for STM32 targets with page/area protection). */
  unlockFlashProtection?(store: FlasherStore): Promise<void>;

  /** Enable flash read protection / RDP Level 1 (optional — advanced/developer tool). */
  lockFlashProtection?(store: FlasherStore): Promise<void>;
}
