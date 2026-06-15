import { defineStore } from "pinia";
import { ref, computed } from "vue";
import type { FlashStrategy, FlasherStore, FlashProtectionStatus, StlinkDeviceInfo, McuFamily } from "@/config/flash-strategy";
import { getDefaultStrategy, getStrategy } from "@/config/flash-strategies";
import { useUiStore, variantToMcuFamily } from "@/stores/ui";

export type { FlashStatus, FlashLogEntry, FlashProtectionStatus, StlinkDeviceInfo, McuFamily } from "@/config/flash-strategy";

export const useFlasherStore = defineStore("flasher", () => {
  const status = ref<import("@/config/flash-strategy").FlashStatus>("idle");
  const progress = ref(0); // 0–100
  const log = ref<import("@/config/flash-strategy").FlashLogEntry[]>([]);
  const chipId = ref<string | null>(null);
  const deviceInfo = ref<StlinkDeviceInfo | null>(null);
  const protectionStatus = ref<FlashProtectionStatus | null>(null);
  const mcuFamily = ref<McuFamily | null>(null);
  const errorMessage = ref<string | null>(null);
  const binaryFile = ref<File | null>(null);
  const verify = ref(true);
  const strategyId = ref<string>(getDefaultStrategy()?.id ?? "webstlink");

  let activeStrategy: FlashStrategy | null = null;

  /** Expected MCU family derived from the hardware variant selection */
  const expectedMcuFamily = computed<McuFamily | null>(() => {
    const ui = useUiStore();
    return ui.hardwareVariant ? variantToMcuFamily(ui.hardwareVariant) : null;
  });

  function addLog(
    level: import("@/config/flash-strategy").FlashLogEntry["level"],
    message: string
  ) {
    log.value.push({ timestamp: Date.now(), level, message });
  }

  /** Build the narrow store interface that strategies operate on */
  function asFlasherStore(): FlasherStore {
    return {
      status, progress, chipId, deviceInfo, protectionStatus,
      mcuFamily, expectedMcuFamily: expectedMcuFamily.value,
      errorMessage, binaryFile, addLog,
    };
  }

  async function connect() {
    const strategy = getStrategy(strategyId.value);
    if (!strategy) {
      addLog("error", `Flash strategy "${strategyId.value}" not found`);
      return;
    }
    activeStrategy = strategy;
    await strategy.connect(asFlasherStore());
    if (status.value === "error") {
      activeStrategy = null;
    }
  }

  async function flash() {
    if (!activeStrategy) {
      throw new Error("Not connected — call connect() first");
    }
    await activeStrategy.flash(asFlasherStore(), verify.value);
  }

  async function unlockProtection() {
    if (!activeStrategy?.unlockFlashProtection) {
      throw new Error("Active strategy does not support flash protection unlock");
    }
    await activeStrategy.unlockFlashProtection(asFlasherStore());
  }

  async function lockProtection() {
    if (!activeStrategy?.lockFlashProtection) {
      throw new Error("Active strategy does not support flash protection lock");
    }
    await activeStrategy.lockFlashProtection(asFlasherStore());
  }

  /**
   * Disconnect the programmer and clear all connection/flash state, returning to
   * idle so the user must reconnect before flashing again. The selected binary
   * file is KEPT. Used by the "Clear / Flash again" button — re-flashing over a
   * live connection is not reliable, so a fresh connect is required each time.
   */
  async function clearForReflash() {
    if (activeStrategy) {
      await activeStrategy.disconnect();
      activeStrategy = null;
    }
    status.value = "idle";
    progress.value = 0;
    log.value = [];
    chipId.value = null;
    deviceInfo.value = null;
    protectionStatus.value = null;
    mcuFamily.value = null;
    errorMessage.value = null;
  }

  /**
   * Full reset: same as clearForReflash() but also clears the selected binary
   * file. Used by the "Reset" button.
   */
  async function reset() {
    await clearForReflash();
    binaryFile.value = null;
  }

  return {
    status,
    progress,
    log,
    chipId,
    deviceInfo,
    protectionStatus,
    mcuFamily,
    expectedMcuFamily,
    errorMessage,
    binaryFile,
    verify,
    strategyId,
    addLog,
    connect,
    flash,
    unlockProtection,
    lockProtection,
    clearForReflash,
    reset,
  };
});
