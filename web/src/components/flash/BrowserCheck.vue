<template>
  <div v-if="webUsbSupported" class="rounded-md bg-green-900/20 border border-green-700/40 p-4 space-y-1">
    <p class="text-sm font-semibold text-green-400 flex items-center gap-2">
      <span class="text-base leading-none" aria-hidden="true">&#x2713;</span>
      In-browser flashing (WebUSB) supported &mdash; ready to flash
    </p>
    <p class="text-xs text-green-300/80">
      You can flash the firmware directly in your web browser &mdash; no account, cloud build, or extra
      software needed. WebUSB works out of the box in Chrome and Edge. Connect your ST-LINK v2 to begin.
    </p>
  </div>
  <div v-else class="rounded-md bg-red-900/20 border border-red-700/40 p-4 space-y-1">
    <p class="text-sm font-semibold text-red-400 flex items-center gap-2">
      <span class="text-base leading-none" aria-hidden="true">&#x2717;</span>
      In-browser flashing (WebUSB) not supported in this browser
    </p>
    <p class="text-xs text-red-300/80">
      In-browser flashing requires Chrome or Edge. In Firefox or Safari, use the OpenOCD command-line
      fallback below instead.
    </p>
  </div>
</template>

<script setup lang="ts">
import { computed } from "vue";

const webUsbSupported = computed(() =>
  typeof navigator !== "undefined" && "usb" in navigator
);
</script>
