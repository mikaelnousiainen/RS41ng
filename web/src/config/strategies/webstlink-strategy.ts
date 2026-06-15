import type {
  FlashStrategy,
  FlasherStore,
  FlashProtectionStatus,
  StlinkDeviceInfo,
  McuFamily,
} from "../flash-strategy";
import type WebStlink from "@/lib/webstlink/webstlink";
import type {
  FlashProtectionInfo,
  FlashProtectionInfoF1,
  FlashProtectionInfoL4,
} from "@/lib/webstlink/webstlink";

const FLASH_START = 0x08000000;
const VERIFY_CHUNK_SIZE = 4096; // 4 KB read-back chunks

/** Sleep helper — used to wait out target resets and option-byte reloads. */
const delay = (ms: number) => new Promise<void>((resolve) => setTimeout(resolve, ms));

/** ST-LINK V2/V2-1/V3 USB device filters */
const STLINK_FILTERS: USBDeviceFilter[] = [
  { vendorId: 0x0483, productId: 0x3748 }, // ST-LINK/V2 standalone (genuine + clones)
  { vendorId: 0x0483, productId: 0x374a }, // ST-LINK/V2-1 (no mass storage variant)
  { vendorId: 0x0483, productId: 0x374b }, // ST-LINK/V2-1 (with VCP + mass storage, Nucleo/Discovery boards)
  { vendorId: 0x0483, productId: 0x374e }, // STLINK-V3E / V3MINIE (embedded + standalone mini)
  { vendorId: 0x0483, productId: 0x374f }, // STLINK-V3S (compact standalone)
  { vendorId: 0x0483, productId: 0x3753 }, // STLINK-V3 dual VCP (no mass storage)
];

/** PID → device model identification */
const STLINK_MODELS: Record<
  number,
  {
    model: string;
    family: StlinkDeviceInfo["family"];
    formFactor: string;
    isLikelyClone: boolean;
  }
> = {
  0x3748: {
    model: "ST-LINK/V2",
    family: "V2",
    formFactor: "Standalone debugger (genuine + clones)",
    isLikelyClone: true,
  },
  0x374a: {
    model: "ST-LINK/V2-1",
    family: "V2-1",
    formFactor: "Board-integrated (no mass storage)",
    isLikelyClone: false,
  },
  0x374b: {
    model: "ST-LINK/V2-1",
    family: "V2-1",
    formFactor: "Board-integrated (Nucleo/Discovery)",
    isLikelyClone: false,
  },
  0x374e: {
    model: "STLINK-V3E",
    family: "V3",
    formFactor: "Embedded / STLINK-V3MINIE",
    isLikelyClone: false,
  },
  0x374f: {
    model: "STLINK-V3S",
    family: "V3",
    formFactor: "Compact standalone",
    isLikelyClone: false,
  },
  0x3753: {
    model: "STLINK-V3",
    family: "V3",
    formFactor: "Dual VCP (no mass storage)",
    isLikelyClone: false,
  },
};

/** Build full device identification from USB PID + firmware version info */
function buildDeviceInfo(
  device: USBDevice,
  ver: NonNullable<WebStlink["version_info"]>
): StlinkDeviceInfo {
  const pidInfo = STLINK_MODELS[device.productId] ?? {
    model: `ST-LINK (0x${device.productId.toString(16)})`,
    family: "V2" as const,
    formFactor: "Unknown",
    isLikelyClone: false,
  };

  return {
    model: pidInfo.model,
    family: pidInfo.family,
    formFactor: pidInfo.formFactor,
    productId: device.productId,
    firmware: ver.str,
    jtagVersion: ver.jtag,
    apiVersion: ver.api,
    swdFreqControl: ver.api >= 3 || ver.jtag >= 22,
    highSpeedUsb: ver.api >= 3,
    isLikelyClone: pidInfo.isLikelyClone,
    firmwareWarning: ver.firmware_warning,
  };
}

/** Derive MCU family from the detected CPU type string */
function detectMcuFamily(cpuType: string): McuFamily | null {
  const t = cpuType.toUpperCase();
  if (
    t.startsWith("STM32F0") ||
    t.startsWith("STM32F1") ||
    t.startsWith("STM32F3")
  ) {
    return "F1";
  }
  if (t.startsWith("STM32L4")) {
    return "L4";
  }
  return null;
}

/**
 * Interpret raw F1 protection registers into structured status.
 * STM32F0xx/F1xx/F3xx: 1KB pages, 4 pages per WRPR group. Bit=0 means protected.
 */
function interpretProtectionF1(
  info: FlashProtectionInfoF1,
  flashSizeKB: number
): FlashProtectionStatus {
  const totalGroups = Math.min(Math.ceil(flashSizeKB / 4), 32);
  let protectedGroups = 0;
  for (let i = 0; i < totalGroups; i++) {
    if (!(info.wrpr_raw & (1 << i))) {
      protectedGroups++;
    }
  }
  const writeProtected = protectedGroups > 0;
  const readProtected = info.rdp_active;

  const parts: string[] = [];
  if (writeProtected) {
    parts.push(
      `${protectedGroups}/${totalGroups} page groups write-protected`
    );
  }
  if (readProtected) {
    parts.push("read protection (RDP Level 1) active");
  }
  const summary =
    parts.length > 0 ? parts.join("; ") : "No flash protection active";

  return {
    writeProtected,
    readProtected,
    protectedGroups,
    totalGroups,
    wrprRaw: info.wrpr_raw,
    summary,
  };
}

/**
 * Interpret raw L4 protection registers into structured status.
 * STM32L4xx: area-based WRP (start/end page ranges). start <= end = protected.
 */
function interpretProtectionL4(
  info: FlashProtectionInfoL4
): FlashProtectionStatus {
  const readProtected = info.rdp_active;
  const wrpAreas: { name: string; startPage: number; endPage: number }[] = [];

  // Area A: protected if start <= end
  if (info.wrp1a_start <= info.wrp1a_end) {
    wrpAreas.push({
      name: "WRP Area A",
      startPage: info.wrp1a_start,
      endPage: info.wrp1a_end,
    });
  }
  // Area B: protected if start <= end
  if (info.wrp1b_start <= info.wrp1b_end) {
    wrpAreas.push({
      name: "WRP Area B",
      startPage: info.wrp1b_start,
      endPage: info.wrp1b_end,
    });
  }

  const writeProtected = wrpAreas.length > 0;

  const parts: string[] = [];
  for (const area of wrpAreas) {
    parts.push(
      `${area.name}: pages ${area.startPage}\u2013${area.endPage} protected`
    );
  }
  if (readProtected) {
    const levelHex = `0x${info.rdp_level.toString(16).padStart(2, "0")}`;
    parts.push(`read protection active (RDP ${levelHex})`);
  }
  const summary =
    parts.length > 0 ? parts.join("; ") : "No flash protection active";

  return {
    writeProtected,
    readProtected,
    summary,
    wrpAreas,
    rdpLevel: info.rdp_level,
    optrRaw: info.optr_raw,
  };
}

/** Interpret protection info from either F1 or L4 format */
function interpretProtection(
  info: FlashProtectionInfo,
  flashSizeKB: number
): FlashProtectionStatus {
  if (info.type === "l4") {
    return interpretProtectionL4(info);
  }
  return interpretProtectionF1(info as FlashProtectionInfoF1, flashSizeKB);
}

/** Log a long string in chunks so the flash log UI does not clip the message. */
function addLogLong(
  store: FlasherStore,
  level: "error" | "warn" | "info",
  prefix: string,
  text: string,
  chunkSize = 900
): void {
  if (text.length <= chunkSize) {
    store.addLog(level, `${prefix}${text}`);
    return;
  }
  const n = Math.ceil(text.length / chunkSize);
  for (let i = 0; i < n; i++) {
    const part = text.slice(i * chunkSize, (i + 1) * chunkSize);
    store.addLog(level, `${prefix}[${i + 1}/${n}] ${part}`);
  }
}

/**
 * Append flash-log lines at 0, 10, …, 100% as each milestone is first reached for the current step.
 */
function logStepPercentMilestones(
  store: FlasherStore,
  stepLabel: "Erase" | "Write" | "Verify",
  percent0to100: number,
  state: { lastLogged: number }
): void {
  const pct = Math.min(100, Math.max(0, percent0to100));
  for (let m = 0; m <= 100; m += 10) {
    if (pct >= m && state.lastLogged < m) {
      store.addLog("info", `${stepLabel}: ${m}%`);
      state.lastLogged = m;
    }
  }
}

type BargraphOpts = { value_min?: number; value_max?: number };

/** Map driver bargraph (erase → write) into store progress 0–88%; optional verify uses 88–100 in flash(). */
function createStlinkLogger(store: FlasherStore) {
  let bgMin = 0;
  let bgMax = 1;
  let phase: "erase" | "write" | "other" = "other";
  const eraseMilestoneState = { lastLogged: -1 };
  const writeMilestoneState = { lastLogged: -1 };

  return {
    info: (msg: string) => store.addLog("info", msg),
    warning: (msg: string) => store.addLog("warn", msg),
    verbose: () => {},
    debug: () => {},
    error: (msg: string) => store.addLog("error", msg),

    bargraph_start(msg: string, opts?: BargraphOpts) {
      if (
        opts &&
        typeof opts.value_min === "number" &&
        typeof opts.value_max === "number"
      ) {
        bgMin = opts.value_min;
        bgMax = opts.value_max;
      } else {
        bgMin = 0;
        bgMax = 1;
      }
      if (msg.includes("Erasing")) {
        phase = "erase";
        store.status.value = "erasing";
        eraseMilestoneState.lastLogged = -1;
        logStepPercentMilestones(store, "Erase", 0, eraseMilestoneState);
      } else if (msg.includes("Writing")) {
        phase = "write";
        store.status.value = "flashing";
        writeMilestoneState.lastLogged = -1;
        logStepPercentMilestones(store, "Write", 0, writeMilestoneState);
      } else phase = "other";
    },

    bargraph_update(payload: { value: number }) {
      const v = payload.value;
      const span = bgMax - bgMin;
      const frac =
        span > 0 ? Math.min(1, Math.max(0, (v - bgMin) / span)) : 0;
      let pct: number;
      if (phase === "erase") {
        const phaseLocal = Math.round(frac * 100);
        logStepPercentMilestones(store, "Erase", phaseLocal, eraseMilestoneState);
        pct = Math.round(frac * 22);
      } else if (phase === "write") {
        const phaseLocal = Math.round(frac * 100);
        logStepPercentMilestones(store, "Write", phaseLocal, writeMilestoneState);
        pct = 22 + Math.round(frac * 66);
      } else pct = Math.round(frac * 88);
      store.progress.value = Math.min(88, pct);
    },

    bargraph_done() {
      if (phase === "erase") {
        store.progress.value = Math.max(store.progress.value, 22);
        logStepPercentMilestones(store, "Erase", 100, eraseMilestoneState);
      } else if (phase === "write") {
        store.progress.value = Math.max(store.progress.value, 88);
        logStepPercentMilestones(store, "Write", 100, writeMilestoneState);
      }
    },
  };
}

export class WebStlinkStrategy implements FlashStrategy {
  readonly id = "webstlink";
  readonly label = "ST-LINK V2/V3 (WebUSB)";

  private stlink: WebStlink | null = null;
  private usbDevice: USBDevice | null = null;

  get available(): boolean {
    return typeof navigator !== "undefined" && "usb" in navigator;
  }

  async connect(store: FlasherStore): Promise<void> {
    store.status.value = "connecting";
    store.addLog("info", "Requesting ST-LINK USB device...");

    try {
      const device = await navigator.usb.requestDevice({
        filters: STLINK_FILTERS,
      });

      // Step 1: Identify USB device by PID (before attach)
      const pidModel = STLINK_MODELS[device.productId];
      if (pidModel) {
        store.addLog(
          "info",
          `USB device: ${pidModel.model} — ${pidModel.formFactor}`
        );
      }

      // Lazy-import to avoid loading webstlink until actually needed
      const { default: WebStlinkClass } = await import(
        "@/lib/webstlink/webstlink.js"
      );

      this.usbDevice = device;
      this.stlink = new WebStlinkClass(createStlinkLogger(store)) as WebStlink;
      await this.stlink.attach(device);
      store.addLog("info", "ST-LINK attached");

      // Step 2: Build full device info from PID + firmware version
      const ver = this.stlink.version_info;
      if (ver) {
        const info = buildDeviceInfo(device, ver);
        store.deviceInfo.value = info;

        store.addLog(
          "info",
          `Firmware: ${info.firmware} (API v${info.apiVersion})` +
            (info.isLikelyClone ? " — likely clone device" : "")
        );

        if (info.firmwareWarning) {
          store.addLog("warn", info.firmwareWarning);
        }
      }

      // Step 3: CPU detection
      const cpu = await this.stlink.detect_cpu([]);

      store.chipId.value = `${cpu.type} (${cpu.flash_size}KB flash)`;
      store.addLog(
        "info",
        `Detected: ${cpu.type}, ${cpu.flash_size}KB flash, ${cpu.sram_size}KB SRAM`
      );

      // Step 4: Determine MCU family
      const family = detectMcuFamily(cpu.type);
      store.mcuFamily.value = family;
      if (family) {
        store.addLog("info", `MCU family: STM32${family}`);
      }

      // Step 5: MCU mismatch detection
      if (store.expectedMcuFamily && family && store.expectedMcuFamily !== family) {
        const expectedLabel =
          store.expectedMcuFamily === "L4"
            ? "STM32L4 (RSM4x4/RSM4x5)"
            : "STM32F1 (RSM4x1/RSM4x2)";
        const detectedLabel =
          family === "L4" ? `STM32L4 (${cpu.type})` : `STM32F1 (${cpu.type})`;
        throw new Error(
          `MCU mismatch: expected ${expectedLabel} but detected ${detectedLabel}. ` +
            `Please check your hardware selection.`
        );
      }

      // Step 6: Flash protection check
      const protection = await this.stlink.read_flash_protection();
      if (protection) {
        const protStatus = interpretProtection(protection, cpu.flash_size);
        store.protectionStatus.value = protStatus;

        if (protStatus.writeProtected || protStatus.readProtected) {
          store.addLog("warn", `Flash protection detected: ${protStatus.summary}`);
        } else {
          store.addLog("info", "Flash protection: none (ready to flash)");
        }
      }

      store.status.value = "connected";
    } catch (err) {
      store.status.value = "error";
      store.errorMessage.value =
        err instanceof Error ? err.message : "Failed to connect";
      store.addLog("error", `Connect failed: ${store.errorMessage.value}`);
      try {
        await this.disconnect();
      } catch {
        /* ignore cleanup errors */
      }
      /* Do not rethrow — Vue event handlers must not get unhandled rejections. */
    }
  }

  async flash(store: FlasherStore, verify: boolean): Promise<void> {
    if (!this.stlink || !store.binaryFile.value) {
      throw new Error("Not connected or no binary file selected");
    }

    try {
      // Read binary file
      const arrayBuffer = await store.binaryFile.value.arrayBuffer();
      const firmware = new Uint8Array(arrayBuffer);
      store.addLog("info", `Firmware size: ${firmware.length} bytes`);

      // Erase + flash (single call, no progress callback)
      store.status.value = "erasing";
      store.progress.value = 0;
      store.addLog("info", "Erasing and writing flash...");

      // Status flips erasing ↔ flashing from ST-LINK bargraph_start (erase vs write phase).
      await this.stlink.flash(FLASH_START, firmware);
      store.progress.value = Math.max(store.progress.value, 88);
      store.addLog("info", "Flash write complete");

      // Optional read-back verification with real progress
      if (verify) {
        store.status.value = "verifying";
        store.progress.value = 0;
        store.addLog("info", "Verifying flash contents...");

        const totalBytes = firmware.length;
        let verified = 0;
        const verifyMilestoneState = { lastLogged: -1 };
        logStepPercentMilestones(store, "Verify", 0, verifyMilestoneState);

        while (verified < totalBytes) {
          const chunkSize = Math.min(VERIFY_CHUNK_SIZE, totalBytes - verified);
          const readBack = await this.stlink.read_memory(
            FLASH_START + verified,
            chunkSize
          );

          for (let i = 0; i < chunkSize; i++) {
            if (readBack[i] !== firmware[verified + i]) {
              const addr = FLASH_START + verified + i;
              throw new Error(
                `Verify failed at 0x${addr.toString(16)}: expected 0x${firmware[verified + i].toString(16)}, got 0x${readBack[i].toString(16)}`
              );
            }
          }

          verified += chunkSize;
          store.progress.value =
            88 + Math.round((verified / totalBytes) * 12);
          const verifyPct =
            totalBytes > 0
              ? Math.round((verified / totalBytes) * 100)
              : 100;
          logStepPercentMilestones(
            store,
            "Verify",
            verifyPct,
            verifyMilestoneState
          );
        }

        logStepPercentMilestones(store, "Verify", 100, verifyMilestoneState);
        store.addLog("info", "Verification passed");
      }

      // Auto-run: reset MCU
      await this.stlink.reset(false);
      store.addLog("info", "MCU reset — firmware running");

      store.status.value = "done";
      store.progress.value = 100;
    } catch (err) {
      store.status.value = "error";
      const raw =
        err instanceof Error ? err.message : String(err ?? "Flash failed");
      store.errorMessage.value = raw;
      addLogLong(store, "error", "Flash failed: ", raw);
      if (err instanceof Error && err.stack && err.stack.length > raw.length) {
        addLogLong(store, "error", "Flash stack: ", err.stack, 900);
      }
      throw err;
    }
  }

  async unlockFlashProtection(store: FlasherStore): Promise<void> {
    if (!this.stlink) {
      throw new Error("Not connected");
    }

    store.status.value = "unlocking";
    store.addLog("info", "Removing flash write protection...");

    try {
      // Erase option bytes + reprogram with protection disabled
      await this.stlink.remove_flash_protection();
      store.addLog(
        "info",
        "Option bytes reprogrammed. Target is resetting and mass-erasing flash..."
      );

      // After removing RDP Level 1, the MCU performs a mass erase of all flash
      // before applying the new option bytes. This can take several seconds on
      // larger flash parts. Wait generously then reconnect.
      await delay(3000);

      if (!(await this.reinitWithRetry(store))) {
        store.addLog("warn", "Quick reinit failed, attempting full reconnect...");
        await this.reconnectFull(store);
        return;
      }

      // Re-detect CPU
      const cpu = await this.stlink.detect_cpu([]);

      store.chipId.value = `${cpu.type} (${cpu.flash_size}KB flash)`;

      // Re-check protection (retry: OBR register can lag after option byte reload)
      let protStatus: FlashProtectionStatus | null = null;
      for (let attempt = 0; attempt < 5; attempt++) {
        if (attempt > 0) {
          await delay(800);
        }
        const protection = await this.stlink.read_flash_protection();
        if (protection) {
          protStatus = interpretProtection(protection, cpu.flash_size);
          store.protectionStatus.value = protStatus;
          if (!protStatus.writeProtected && !protStatus.readProtected) {
            break;
          }
        }
      }
      if (
        protStatus &&
        (protStatus.writeProtected || protStatus.readProtected)
      ) {
        throw new Error(
          "Flash protection still active after unlock — removal may have failed"
        );
      }

      store.addLog("info", "Flash protection removed successfully");
      store.status.value = "connected";
    } catch (err) {
      store.status.value = "error";
      store.errorMessage.value =
        err instanceof Error ? err.message : "Unlock failed";
      store.addLog("error", `Unlock failed: ${store.errorMessage.value}`);
      throw err;
    }
  }

  async lockFlashProtection(store: FlasherStore): Promise<void> {
    if (!this.stlink) {
      throw new Error("Not connected");
    }

    // Reuse the "unlocking" status enum for any option-byte operation — there is
    // no dedicated "locking" state, and the UI treats both the same way.
    store.status.value = "unlocking";
    store.addLog("info", "Enabling flash read protection (RDP Level 1)...");

    try {
      await this.stlink.enable_flash_protection();
      store.addLog(
        "info",
        "Option bytes reprogrammed with RDP Level 1. Target is resetting..."
      );

      // Wait for reset to complete
      await delay(2000);

      if (!(await this.reinitWithRetry(store))) {
        store.addLog("warn", "Quick reinit failed, attempting full reconnect...");
        await this.reconnectFull(store);
        return;
      }

      const { cpu } = await this.refreshTargetState(store);

      // F1 family warning: power cycle needed
      const family = detectMcuFamily(cpu.type);
      if (family === "F1") {
        store.addLog(
          "warn",
          "STM32F1 requires a POWER CYCLE for RDP to take effect. " +
            "Disconnect power, wait a moment, then reconnect."
        );
      }

      store.addLog("info", "Flash read protection (RDP Level 1) enabled");
      store.status.value = "connected";
    } catch (err) {
      store.status.value = "error";
      store.errorMessage.value =
        err instanceof Error ? err.message : "Lock failed";
      store.addLog("error", `Lock failed: ${store.errorMessage.value}`);
      throw err;
    }
  }

  /**
   * Re-establish the debug session after the target resets (option-byte reload).
   * Tries a quick reinit a few times; returns false if it never reconnects so the
   * caller can fall back to a full re-attach.
   */
  private async reinitWithRetry(store: FlasherStore): Promise<boolean> {
    if (!this.stlink) return false;
    for (let attempt = 0; attempt < 3; attempt++) {
      try {
        if (attempt > 0) {
          store.addLog("info", `Reconnect attempt ${attempt + 1}...`);
          await delay(1500);
        } else {
          store.addLog("info", "Reconnecting to target...");
        }
        await this.stlink.reinit_debug();
        return true;
      } catch {
        // reinit can fail if MCU is still in reset — retry
      }
    }
    return false;
  }

  /** Re-detect the CPU and read protection into the store; returns both. */
  private async refreshTargetState(
    store: FlasherStore
  ): Promise<{ cpu: Awaited<ReturnType<WebStlink["detect_cpu"]>>; protStatus: FlashProtectionStatus | null }> {
    const cpu = await this.stlink!.detect_cpu([]);
    store.chipId.value = `${cpu.type} (${cpu.flash_size}KB flash)`;

    let protStatus: FlashProtectionStatus | null = null;
    const protection = await this.stlink!.read_flash_protection();
    if (protection) {
      protStatus = interpretProtection(protection, cpu.flash_size);
      store.protectionStatus.value = protStatus;
    }
    return { cpu, protStatus };
  }

  /** Full reconnect fallback: detach + re-attach + re-detect. */
  private async reconnectFull(store: FlasherStore): Promise<void> {
    if (!this.stlink || !this.usbDevice) {
      throw new Error("Cannot reconnect — no USB device reference");
    }

    store.addLog("info", "Full reconnect: detaching...");
    try {
      await this.stlink.detach();
    } catch {
      /* ignore */
    }

    const { default: WebStlinkClass } = await import(
      "@/lib/webstlink/webstlink.js"
    );

    this.stlink = new WebStlinkClass(createStlinkLogger(store)) as WebStlink;
    await this.stlink.attach(this.usbDevice);
    store.addLog("info", "ST-LINK re-attached");

    const { protStatus } = await this.refreshTargetState(store);
    if (protStatus && (protStatus.writeProtected || protStatus.readProtected)) {
      throw new Error("Protection still active after full reconnect");
    }

    store.addLog("info", "Flash protection removed (full reconnect)");
    store.status.value = "connected";
  }

  async disconnect(): Promise<void> {
    if (this.stlink) {
      try {
        await this.stlink.detach();
      } catch {
        // Ignore detach errors during cleanup
      }
      this.stlink = null;
    }
    this.usbDevice = null;
  }
}
