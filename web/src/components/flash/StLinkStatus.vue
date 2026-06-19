<template>
  <div class="space-y-3">
    <!-- What to do before connecting -->
    <p class="text-sm text-gray-400">
      After wiring the ST-LINK v2 to your radiosonde, plug the ST-LINK into a USB port, then click
      <span class="font-medium text-gray-200">Connect ST-LINK</span> to detect the programmer.
    </p>

    <!-- Connect button (always visible, disabled once connected) with status indicator to its right -->
    <div class="flex items-center gap-3 flex-wrap">
      <button
        type="button"
        class="shrink-0 px-4 py-2 text-sm font-medium bg-green-700 hover:bg-green-600 text-white rounded-md transition-colors disabled:opacity-50 disabled:cursor-not-allowed"
        :disabled="flasher.status !== 'idle'"
        @click="flasher.connect()"
      >
        Connect ST-LINK
      </button>
      <div class="flex items-center gap-2 text-sm">
        <div
          class="w-2 h-2 rounded-full shrink-0 transition-colors"
          :class="statusColor"
        />
        <span class="text-gray-300 min-w-0">{{ statusText }}</span>
      </div>
    </div>

    <!-- Device identification (shown after connect) -->
    <div v-if="flasher.deviceInfo" class="space-y-1">
      <p class="text-xs text-gray-400">
        {{ flasher.deviceInfo.model }}
        <span class="font-mono text-gray-500">{{ flasher.deviceInfo.firmware }}</span>
        <span class="text-gray-600"> · </span>
        {{ flasher.deviceInfo.formFactor }}
        <template v-if="flasher.deviceInfo.isLikelyClone">
          <span class="text-gray-600"> · </span>
          <span class="text-yellow-500">likely clone</span>
        </template>
        <span class="text-gray-600"> · </span>
        API v{{ flasher.deviceInfo.apiVersion }}
      </p>
      <p v-if="flasher.chipId" class="font-mono text-xs text-gray-500">
        {{ flasher.chipId }}
      </p>

      <!-- Condensed firmware warning -->
      <p v-if="condensedWarning" class="text-xs text-amber-400/80">
        &#x26A0; {{ condensedWarning }}
      </p>

      <!-- Flash protection warning -->
      <div v-if="isProtected" class="mt-2 rounded-md bg-amber-900/30 border border-amber-700/50 px-3 py-2 space-y-2">
        <div class="flex items-start gap-2">
          <span class="text-amber-400 text-sm shrink-0">&#x1F512;</span>
          <div class="space-y-1 min-w-0">
            <p class="text-xs text-amber-300">
              Flash write protection is active
            </p>
            <p class="text-xs text-amber-400/70">
              {{ flasher.protectionStatus?.summary }}
            </p>

            <button
              v-if="!showUnlockConfirm"
              type="button"
              class="mt-1 px-2.5 py-1 text-xs font-medium bg-amber-700 hover:bg-amber-600 text-white rounded transition-colors"
              @click="showUnlockConfirm = true"
            >
              Remove protection
            </button>

            <!-- Inline confirmation panel -->
            <div v-if="showUnlockConfirm" class="mt-2 rounded bg-gray-950 border border-gray-700 p-3 space-y-2">
              <p class="text-xs text-gray-300">
                This will erase and reprogram the STM32 option bytes to remove all flash protection.
                The MCU will reset automatically afterward.
              </p>

              <details class="text-xs text-gray-500">
                <summary class="cursor-pointer hover:text-gray-400 select-none">
                  Show technical details
                </summary>
                <!-- F1-specific technical details -->
                <ul v-if="isF1Family" class="mt-1.5 ml-4 list-disc space-y-0.5 text-gray-500">
                  <li>FLASH_WRPR: 0x{{ wrprHex }}</li>
                  <li>Protected groups: {{ flasher.protectionStatus?.protectedGroups }}/{{ flasher.protectionStatus?.totalGroups }}</li>
                  <li v-if="flasher.protectionStatus?.readProtected" class="text-red-400">
                    RDP Level 1 is active &mdash; erasing option bytes will trigger a mass erase of all flash memory
                  </li>
                  <li>Option bytes at 0x1FFFF800 will be erased and reprogrammed</li>
                  <li>RDP set to Level 0 (0xA5), WRP0-3 set to 0xFF (unprotected)</li>
                  <li>MCU will reset via AIRCR.SYSRESETREQ to reload option bytes</li>
                </ul>
                <!-- L4-specific technical details -->
                <ul v-else class="mt-1.5 ml-4 list-disc space-y-0.5 text-gray-500">
                  <li>FLASH_OPTR: 0x{{ optrHex }}</li>
                  <li v-for="area in flasher.protectionStatus?.wrpAreas" :key="area.name">
                    {{ area.name }}: pages {{ area.startPage }}&ndash;{{ area.endPage }}
                  </li>
                  <li v-if="flasher.protectionStatus?.readProtected" class="text-red-400">
                    RDP is active (level 0x{{ rdpLevelHex }}) &mdash; removing it will trigger a mass erase of all flash memory
                  </li>
                  <li>OPTR will be reprogrammed with RDP = 0xAA (Level 0)</li>
                  <li>WRP1AR/WRP1BR set to no-protection (start &gt; end)</li>
                  <li>MCU will reset via OBL_LAUNCH to reload option bytes</li>
                </ul>
              </details>

              <p v-if="flasher.protectionStatus?.readProtected" class="text-xs text-red-400 font-medium">
                Read protection is active. Removing it will mass-erase all flash contents.
                This is expected &mdash; you will flash new firmware afterward.
              </p>

              <div class="flex gap-2 pt-1">
                <button
                  type="button"
                  class="px-3 py-1.5 text-xs font-medium bg-amber-600 hover:bg-amber-500 text-white rounded transition-colors"
                  :disabled="flasher.status === 'unlocking'"
                  @click="doUnlock"
                >
                  {{ flasher.status === 'unlocking' ? 'Unlocking\u2026' : 'Confirm unlock' }}
                </button>
                <button
                  type="button"
                  class="px-3 py-1.5 text-xs font-medium bg-gray-800 hover:bg-gray-700 text-gray-300 rounded transition-colors"
                  :disabled="flasher.status === 'unlocking'"
                  @click="showUnlockConfirm = false"
                >
                  Cancel
                </button>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- Developer tools: Lock MCU (collapsed by default) -->
      <details
        v-if="canLock"
        class="mt-3 text-xs"
      >
        <summary class="cursor-pointer text-gray-500 hover:text-gray-400 select-none">
          Developer tools
          <span class="ml-1 px-1.5 py-0.5 rounded bg-amber-900/40 text-amber-400/90 text-[10px] font-medium uppercase tracking-wide">
            Experimental
          </span>
        </summary>
        <div class="mt-2 rounded-md bg-gray-800/50 border border-gray-700/50 px-3 py-2 space-y-2">
          <p class="text-amber-400/80">
            Experimental — not needed for normal use. The RDP write may not take effect on all STM32F1 clones.
          </p>
          <p class="text-gray-400">
            Lock MCU &mdash; enable flash read protection (RDP Level 1).
            This prevents reading flash contents via the debug port.
          </p>
          <p class="text-yellow-500/80">
            After locking, the MCU will require an unlock (which triggers mass erase) before it can be read or re-flashed.
          </p>
          <p v-if="isF1Family" class="text-amber-400/70">
            Note: STM32F1 requires a power cycle after locking for RDP to take effect.
          </p>

          <button
            v-if="!showLockConfirm"
            type="button"
            class="mt-1 px-2.5 py-1 text-xs font-medium bg-gray-700 hover:bg-gray-600 text-white rounded transition-colors"
            @click="showLockConfirm = true"
          >
            Lock MCU (enable RDP)
          </button>

          <!-- Lock confirmation panel -->
          <div v-if="showLockConfirm" class="mt-2 rounded bg-gray-950 border border-gray-700 p-3 space-y-2">
            <p class="text-gray-300">
              Are you sure? This will enable RDP Level 1 read protection on the MCU.
            </p>
            <p class="text-gray-500">
              Flash contents will be preserved (no mass erase), but the debug port will no longer be able to read flash.
              To undo this, you will need to unlock (which mass-erases flash) and re-flash firmware.
            </p>
            <div class="flex gap-2 pt-1">
              <button
                type="button"
                class="px-3 py-1.5 text-xs font-medium bg-amber-700 hover:bg-amber-600 text-white rounded transition-colors"
                :disabled="flasher.status === 'unlocking'"
                @click="doLock"
              >
                {{ flasher.status === 'unlocking' ? 'Locking\u2026' : 'Confirm lock' }}
              </button>
              <button
                type="button"
                class="px-3 py-1.5 text-xs font-medium bg-gray-800 hover:bg-gray-700 text-gray-300 rounded transition-colors"
                :disabled="flasher.status === 'unlocking'"
                @click="showLockConfirm = false"
              >
                Cancel
              </button>
            </div>
          </div>
        </div>
      </details>
    </div>

    <!-- chipId fallback (shown if no deviceInfo but chipId exists) -->
    <div v-else-if="flasher.chipId">
      <span class="font-mono text-xs text-gray-500">{{ flasher.chipId }}</span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, ref } from "vue";
import { useFlasherStore } from "@/stores/flasher";

const flasher = useFlasherStore();
const showUnlockConfirm = ref(false);
const showLockConfirm = ref(false);

const statusColor = computed(() => {
  switch (flasher.status) {
    case "idle":
      return "bg-gray-600";
    case "connecting":
      return "bg-yellow-400 animate-pulse";
    case "unlocking":
      return "bg-amber-400 animate-pulse";
    case "connected":
    case "erasing":
    case "flashing":
    case "verifying":
      return "bg-green-400";
    case "done":
      return "bg-green-600";
    case "error":
      return "bg-red-500";
    default:
      return "bg-gray-600";
  }
});

const statusText = computed(() => {
  switch (flasher.status) {
    case "idle":
      return "Not connected";
    case "connecting":
      return "Connecting to ST-LINK\u2026";
    case "unlocking":
      return "Removing flash protection\u2026";
    case "connected":
      return "Connected";
    case "erasing":
      return "Erasing flash\u2026";
    case "flashing":
      return "Writing firmware\u2026";
    case "verifying":
      return "Verifying\u2026";
    case "done":
      return "Flash complete";
    case "error":
      return flasher.errorMessage ?? "Error";
    default:
      return "Unknown";
  }
});

const condensedWarning = computed(() => {
  const info = flasher.deviceInfo;
  if (!info?.firmwareWarning) return null;
  const parts = [`Older firmware (J${info.jtagVersion})`];
  if (!info.swdFreqControl) parts.push("SWD frequency control unavailable");
  if (info.isLikelyClone) parts.push("Do not upgrade clone firmware");
  return parts.join(" · ");
});

const isProtected = computed(() => {
  const ps = flasher.protectionStatus;
  return ps != null && (ps.writeProtected || ps.readProtected);
});

/** Whether detected MCU is F1 family (affects technical details display) */
const isF1Family = computed(() => flasher.mcuFamily === "F1");

const wrprHex = computed(() => {
  const raw = flasher.protectionStatus?.wrprRaw ?? 0;
  return raw.toString(16).padStart(8, "0");
});

const optrHex = computed(() => {
  const raw = flasher.protectionStatus?.optrRaw ?? 0;
  return raw.toString(16).padStart(8, "0");
});

const rdpLevelHex = computed(() => {
  const raw = flasher.protectionStatus?.rdpLevel ?? 0;
  return raw.toString(16).padStart(2, "0");
});

/** Show lock button when connected and protection is NOT active */
const canLock = computed(() => {
  const connected = flasher.status === "connected";
  const ps = flasher.protectionStatus;
  const notProtected = ps != null && !ps.writeProtected && !ps.readProtected;
  return connected && notProtected;
});

async function doUnlock() {
  try {
    await flasher.unlockProtection();
    showUnlockConfirm.value = false;
  } catch {
    // Error already logged and displayed by the strategy
  }
}

async function doLock() {
  try {
    await flasher.lockProtection();
    showLockConfirm.value = false;
  } catch {
    // Error already logged and displayed by the strategy
  }
}
</script>
