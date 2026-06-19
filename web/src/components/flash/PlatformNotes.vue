<template>
  <div v-if="note" class="rounded-md bg-gray-900 border border-gray-800 p-4 space-y-2">
    <p class="text-xs font-semibold text-gray-400 uppercase tracking-wider">
      Detected OS: {{ note.osName }} &mdash; {{ note.description }}
    </p>
    <div class="text-xs text-gray-500 space-y-2" v-html="note.html" />
  </div>
</template>

<script setup lang="ts">
import { computed } from "vue";
import { useUiStore } from "@/stores/ui";

const ui = useUiStore();

const NOTES: Partial<Record<string, { osName: string; description: string; html: string }>> = {
  linux: {
    osName: "Linux",
    description: "udev rule required for in-browser flashing (WebUSB)",
    html: `<p>Add a udev rule so Chrome can access the ST-LINK without root:</p>
<pre class="font-mono bg-gray-950 rounded p-2 mt-1 text-green-300 overflow-x-auto">echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="0483", MODE="0666"' | sudo tee /etc/udev/rules.d/99-stlink.rules
sudo udevadm control --reload-rules</pre>
<p class="mt-1">Then unplug and replug the ST-LINK.</p>`,
  },
  windows: {
    osName: "Windows",
    description: "WinUSB driver required for in-browser flashing (WebUSB)",
    html: `<p>Install the WinUSB driver using <a href="https://zadig.akeo.ie/" target="_blank" rel="noopener" class="text-green-400 hover:underline">Zadig</a>:</p>
<ol class="list-decimal ml-4 space-y-0.5 mt-1">
<li>Open Zadig and select your ST-LINK V2 device</li>
<li>Choose <strong>WinUSB</strong> as the driver</li>
<li>Click <strong>Install Driver</strong></li>
</ol>`,
  },
  macos: {
    osName: "macOS",
    description: "no setup required for in-browser flashing (WebUSB)",
    html: `<p>In-browser flashing (WebUSB) works out of the box in Chrome and Edge on macOS. No additional setup required.</p>`,
  },
};

const note = computed(() => NOTES[ui.os] ?? null);
</script>
