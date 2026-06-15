<template>
  <details class="group rounded-md border border-gray-800" :open="!webUsbSupported">
    <summary
      class="flex items-center gap-3 px-4 py-3 cursor-pointer select-none text-sm text-gray-400 hover:text-gray-300 transition-colors"
    >
      <svg
        class="w-3.5 h-3.5 shrink-0 transition-transform group-open:rotate-90 text-gray-500"
        fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="2"
      >
        <path d="M9 5l7 7-7 7" />
      </svg>
      <span>Alternative method: OpenOCD + ST-LINK</span>
    </summary>

    <div class="px-4 pb-4 space-y-4 border-t border-gray-800 pt-4">
      <p class="text-xs text-gray-500">
        Flash from the command line with OpenOCD and an ST-LINK v2 instead of WebUSB. The commands below are for
        <span class="text-gray-300">{{ hardwareLabel }}</span>
        ({{ family === "L4" ? "STM32L4" : "STM32F1" }}-based board).
      </p>

      <!-- STM32F1: older RS41 (RSM4x1/4x2) and DFM-17 -->
      <template v-if="family === 'F1'">
        <div class="space-y-2">
          <p class="text-xs font-medium text-gray-400">1. Unlock flash protection (first time only):</p>
          <CommandBlock
            command='openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "init; halt; flash protect 0 0 63 off; exit"'
            label="unlock"
          />
          <p class="text-xs text-gray-500">
            If this fails with an error about erasing sectors, replace <code class="font-mono">63</code> with
            <code class="font-mono">31</code> (or the number the error message suggests).
          </p>
        </div>

        <div class="space-y-2">
          <p class="text-xs font-medium text-gray-400">2. Flash the firmware:</p>
          <CommandBlock
            command='openocd -f interface/stlink.cfg -f target/stm32f1x.cfg -c "program build/src/RS41ng.elf verify reset exit"'
            label="flash"
          />
        </div>

        <p class="text-xs text-gray-500">3. Power-cycle the sonde to start running the new firmware.</p>
      </template>

      <!-- STM32L4: newer RS41 (RSM4x4/4x5) -->
      <template v-else>
        <div class="space-y-2">
          <p class="text-xs font-medium text-gray-400">1. Unlock flash protection (first time only):</p>
          <CommandBlock
            command='openocd -f interface/stlink.cfg -f target/stm32l4x.cfg -c "init; reset halt; stm32l4x unlock 0; reset halt; exit"'
            label="unlock 1 of 2"
          />
          <p class="text-xs text-gray-500">
            Expect a line containing <code class="font-mono">RDP level 1 (0x00)</code>. Then disconnect the sonde,
            remove the batteries if present, wait ~30 seconds for the capacitors to discharge, and reconnect.
          </p>
          <CommandBlock
            command='openocd -f interface/stlink.cfg -f target/stm32l4x.cfg -c "init; reset halt; flash protect 0 0 last off; exit"'
            label="unlock 2 of 2"
          />
          <p class="text-xs text-gray-500">
            Expect <code class="font-mono">RDP level 0 (0xAA)</code> — the board is now unlocked. If you don't see it,
            repeat from the first command. Disconnect, discharge ~30 seconds, and reconnect again before flashing.
          </p>
        </div>

        <div class="space-y-2">
          <p class="text-xs font-medium text-gray-400">2. Flash the firmware:</p>
          <CommandBlock
            command='openocd -f interface/stlink.cfg -f target/stm32l4x.cfg -c "program build/src/RS41ng.elf verify reset exit"'
            label="flash"
          />
        </div>

        <p class="text-xs text-gray-500">3. Power-cycle the sonde to start running the new firmware.</p>
      </template>

      <p class="text-xs text-gray-600">
        See the
        <a
          href="https://github.com/mikaelnousiainen/RS41ng#flashing-the-radiosonde-with-the-firmware"
          target="_blank"
          rel="noopener"
          class="text-green-400 hover:underline"
        >README flashing section</a>
        for full details.
      </p>
    </div>
  </details>
</template>

<script setup lang="ts">
import { computed } from "vue";
import { useUiStore, variantToMcuFamily } from "@/stores/ui";
import { isWebUsbSupported } from "@/utils/webusb";
import CommandBlock from "@/components/build/CommandBlock.vue";

const ui = useUiStore();

const HARDWARE_LABELS: Record<string, string> = {
  RS41: "RS41 (RSM4x1/RSM4x2)",
  RS41_RSM4x4: "RS41 (RSM4x4/RSM4x5)",
  DFM17: "DFM-17",
};

// Auto-expand this fallback when WebUSB is unavailable (the in-browser flasher is
// hidden in that case, so OpenOCD is the only option).
const webUsbSupported = computed(() => isWebUsbSupported());

// The flash step is only reachable once hardware is selected; default to F1 defensively.
const family = computed(() => (ui.hardwareVariant ? variantToMcuFamily(ui.hardwareVariant) : "F1"));
const hardwareLabel = computed(() =>
  ui.hardwareVariant ? HARDWARE_LABELS[ui.hardwareVariant] : "your sonde"
);
</script>
