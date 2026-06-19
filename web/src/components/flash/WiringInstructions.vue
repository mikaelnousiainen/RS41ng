<template>
  <details v-if="wiring" class="group rounded-lg border border-gray-800 bg-gray-900">
    <summary
      class="flex items-start gap-3 px-4 py-3 cursor-pointer select-none text-sm text-gray-300 hover:text-gray-100 transition-colors"
    >
      <svg
        class="mt-0.5 w-3.5 h-3.5 shrink-0 transition-transform group-open:rotate-90 text-gray-500"
        fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="2"
      >
        <path d="M9 5l7 7-7 7" />
      </svg>
      <span class="min-w-0">
        <span class="block font-medium">ST-LINK v2 wiring for {{ wiring.title }}</span>
        <span class="mt-0.5 block text-xs text-gray-500">
          Connect your {{ wiring.title }} radiosonde to the ST-LINK v2 adapter before flashing &mdash;
          click here to view the wiring instructions.
        </span>
      </span>
    </summary>

    <div class="px-4 pb-4 space-y-4 border-t border-gray-800 pt-4">
      <!-- Pin mapping table -->
      <table class="w-full text-sm">
        <thead>
          <tr class="text-left text-xs text-gray-500 uppercase tracking-wider">
            <th class="pb-2 pr-4">ST-LINK</th>
            <th class="pb-2 pr-4">{{ wiring.connectorLabel }} Pin</th>
            <th class="pb-2">Signal</th>
          </tr>
        </thead>
        <tbody class="text-gray-300">
          <tr v-for="pin in wiring.pins" :key="pin.stlink" class="border-t border-gray-800">
            <td class="py-1.5 pr-4 font-mono text-xs">{{ pin.stlink }}</td>
            <td class="py-1.5 pr-4 font-mono text-xs">{{ pin.sonde }}</td>
            <td class="py-1.5 text-xs text-gray-400">{{ pin.signal }}</td>
          </tr>
        </tbody>
      </table>

      <!-- Connector diagram -->
      <div>
        <p class="text-xs font-medium text-gray-400 mb-2">Connector pinout</p>
        <pre class="font-mono text-xs text-gray-500 bg-gray-950 rounded p-3 overflow-x-auto leading-relaxed">{{ wiring.diagram }}</pre>
      </div>

      <!-- Power note -->
      <p class="text-xs text-gray-500">{{ wiring.powerNote }}</p>

      <!-- Soldering note (DFM-17 only) -->
      <p v-if="wiring.solderingNote" class="text-xs text-yellow-500/80">
        {{ wiring.solderingNote }}
      </p>

      <!-- Image placeholder -->
      <div class="rounded border border-dashed border-gray-700 bg-gray-950/50 p-6 text-center">
        <svg class="mx-auto w-10 h-10 text-gray-700 mb-1" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="1">
          <path d="M4 16l4.586-4.586a2 2 0 012.828 0L16 16m-2-2l1.586-1.586a2 2 0 012.828 0L20 14m-6-6h.01M6 20h12a2 2 0 002-2V6a2 2 0 00-2-2H6a2 2 0 00-2 2v12a2 2 0 002 2z" />
        </svg>
        <p class="text-xs text-gray-600">{{ wiring.imagePlaceholder }}</p>
      </div>
    </div>
  </details>
</template>

<script setup lang="ts">
import { computed } from "vue";
import { useUiStore } from "@/stores/ui";

const ui = useUiStore();

interface PinMapping {
  stlink: string;
  sonde: string;
  signal: string;
}

interface WiringInfo {
  title: string;
  connectorLabel: string;
  pins: PinMapping[];
  diagram: string;
  powerNote: string;
  solderingNote?: string;
  imagePlaceholder: string;
}

const RS41_DIAGRAM = `______________________|           |______________________
|                                                       |
|   9           7           5           3           1   |
|                                                       |
|   10          8           6           4           2   |
|_______________________________________________________|

(View from the bottom of the sonde, pay attention to the gap in the connector)`;

const DFM17_DIAGRAM = `_____
|   |
| S |       _____________________
| e |       |                   |
| n |       |   1           2   |
| s |       |                   |
| o |       |   3           4   |
| r |       |                   |
|   |       |   5           6   |
| b |       |                   |
| o |       |   7           8   |
| o |       |                   |
| m |       |   9          10   |
|   |       |                   |
|   |       |____________________
|   |
|___|

(Sensor boom on the left, microcontroller on the right)`;

const RS41_WIRING: WiringInfo = {
  title: "RS41",
  connectorLabel: "RS41",
  pins: [
    { stlink: "SWDIO", sonde: "Pin 9", signal: "SWDIO (PA13)" },
    { stlink: "SWCLK", sonde: "Pin 8", signal: "SWCLK (PA14)" },
    { stlink: "GND", sonde: "Pin 1", signal: "GND" },
    {
      stlink: "3.3V",
      sonde: "Pin 5",
      signal: "MCU switched 3.3V out",
    },
  ],
  diagram: RS41_DIAGRAM,
  powerNote:
    "If your ST-LINK v2 can provide 3.3V, remove the batteries before flashing. Otherwise, leave batteries in and power on the sonde.",
  imagePlaceholder: "Wiring photo for RS41 ST-LINK v2 connection",
};

const DFM17_WIRING: WiringInfo = {
  title: "DFM-17",
  connectorLabel: "DFM-17",
  pins: [
    { stlink: "SWDIO", sonde: "Pin 2", signal: "SWDIO / TMS" },
    { stlink: "SWCLK", sonde: "Pin 4", signal: "SWCLK / TCK" },
    { stlink: "RST", sonde: "Pin 10", signal: "nRESET" },
    { stlink: "GND", sonde: "Pin 5", signal: "GND" },
    { stlink: "3.3V", sonde: "Pin 1", signal: "VTRef" },
  ],
  diagram: DFM17_DIAGRAM,
  powerNote:
    "If your ST-LINK v2 can provide 3.3V, remove the batteries before flashing. Otherwise, leave batteries in and power on the sonde.",
  solderingNote:
    "The DFM-17 programming connector is an unpopulated group of pads on the circuit board. You must solder wires directly to the pads or solder a 0.05\" (1.27 mm) 5-by-2 pin header.",
  imagePlaceholder: "Wiring photo for DFM-17 ST-LINK v2 connection",
};

const wiring = computed((): WiringInfo | null => {
  switch (ui.hardwareVariant) {
    case "RS41":
    case "RS41_RSM4x4":
      return RS41_WIRING;
    case "DFM17":
      return DFM17_WIRING;
    default:
      return null;
  }
});
</script>
