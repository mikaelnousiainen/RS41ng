<template>
  <div class="space-y-6">
    <div>
      <h1 class="text-xl font-semibold text-gray-100">Flash Firmware</h1>
      <p class="mt-1 text-sm text-gray-400">
        Flash your built firmware to the radiosonde via WebUSB + ST-LINK v2.
      </p>
    </div>

    <BrowserCheck />

    <!-- WebUSB driver setup is only relevant when WebUSB is available -->
    <PlatformNotes v-if="webUsbSupported" />

    <WiringInstructions />

    <!-- WebUSB flasher: hidden when WebUSB is unsupported (use the OpenOCD fallback below) -->
    <section
      v-if="webUsbSupported"
      class="rounded-lg bg-gray-900 border border-gray-800 p-4 space-y-4"
    >
      <StLinkStatus />

      <FilePicker />

      <div class="flex items-center gap-3 flex-wrap">
        <button
          v-if="flasher.status !== 'done'"
          type="button"
          class="px-4 py-2 text-sm font-medium bg-green-700 hover:bg-green-600 text-white rounded-md transition-colors disabled:opacity-50 disabled:cursor-not-allowed"
          :disabled="flasher.status !== 'connected' || !flasher.binaryFile || isProtected"
          @click="startFlash"
        >
          Flash firmware
        </button>
        <button
          v-else
          type="button"
          class="px-4 py-2 text-sm font-medium bg-yellow-600 hover:bg-yellow-500 text-gray-900 rounded-md transition-colors"
          @click="flasher.clearForReflash()"
        >
          Clear / Flash again
        </button>
        <label class="flex items-center gap-2 text-sm text-gray-400">
          <input
            v-model="flasher.verify"
            type="checkbox"
            class="rounded bg-gray-800 border-gray-600 text-green-500 focus:ring-green-500 focus:ring-offset-0"
          />
          Verify after flash
        </label>
        <button
          v-if="flasher.status !== 'idle' && flasher.status !== 'done'"
          type="button"
          class="ml-auto px-3 py-2 text-sm font-medium bg-gray-800 hover:bg-gray-700 text-gray-300 rounded-md transition-colors"
          @click="flasher.reset()"
        >
          Reset
        </button>
      </div>

      <FlashProgress v-if="flasher.status !== 'idle'" />
      <FlashLog />
    </section>

    <!-- OpenOCD fallback -->
    <FallbackInstructions />

    <!-- Navigation -->
    <div class="flex justify-start pt-4 border-t border-gray-800">
      <button
        type="button"
        class="px-4 py-2 text-sm font-medium bg-gray-800 hover:bg-gray-700 text-gray-300 rounded-md transition-colors"
        @click="ui.goBack()"
      >
        &larr; Back
      </button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from "vue";
import { useUiStore } from "@/stores/ui";
import { useFlasherStore } from "@/stores/flasher";
import BrowserCheck from "@/components/flash/BrowserCheck.vue";
import PlatformNotes from "@/components/flash/PlatformNotes.vue";
import StLinkStatus from "@/components/flash/StLinkStatus.vue";
import FilePicker from "@/components/flash/FilePicker.vue";
import FlashProgress from "@/components/flash/FlashProgress.vue";
import FlashLog from "@/components/flash/FlashLog.vue";
import FallbackInstructions from "@/components/flash/FallbackInstructions.vue";
import WiringInstructions from "@/components/flash/WiringInstructions.vue";
import { isWebUsbSupported } from "@/utils/webusb";

const ui = useUiStore();
const flasher = useFlasherStore();

// WebUSB availability gates the in-browser flasher UI; without it, only the
// OpenOCD command-line fallback is usable.
const webUsbSupported = computed(() => isWebUsbSupported());

const isProtected = computed(() => {
  const ps = flasher.protectionStatus;
  return ps != null && (ps.writeProtected || ps.readProtected);
});

async function startFlash() {
  try {
    await flasher.flash();
  } catch {
    // Error already logged by strategy — no additional handling needed
  }
}
</script>
