<template>
  <div class="space-y-6">
    <!-- Intro -->
    <div class="text-sm text-gray-400 leading-relaxed">
      <p>
        This browser-based tool helps you configure, build, and flash
        <strong class="text-gray-200">RS41ng</strong> &mdash; open-source amateur-radio firmware for the
        Vaisala RS41 and Graw DFM-17 radiosondes. It reprograms a recovered weather-balloon radiosonde into a
        multi-mode transmitter (APRS, Horus 4FSK, CW, WSPR and more) for high-altitude balloon tracking and
        radio experiments.
      </p>
      <p class="mt-2">
        For full documentation, supported features and source code, see the
        <a
          href="https://github.com/mikaelnousiainen/RS41ng"
          target="_blank"
          rel="noopener"
          class="text-green-400 hover:underline"
        >RS41ng project on GitHub &rarr;</a>
      </p>
    </div>

    <div>
      <h1 class="text-xl font-semibold text-gray-100">Select Hardware</h1>
      <p class="mt-1 text-sm text-gray-400">
        Choose your radiosonde type. Check the PCB marking or serial number to identify your hardware revision.
      </p>
    </div>

    <!-- Already-have-a-binary shortcut -->
    <div class="rounded-md bg-blue-900/20 border border-blue-700/40 p-4 flex items-start gap-3">
      <svg class="w-5 h-5 text-blue-400 shrink-0 mt-0.5" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="2">
        <circle cx="12" cy="12" r="9" />
        <path d="M12 11v5M12 8h.01" stroke-linecap="round" />
      </svg>
      <div class="text-sm text-blue-200/90 space-y-0.5">
        <p class="font-medium text-blue-300">Already have a firmware binary?</p>
        <p>
          If you already have an <code class="font-mono">RS41ng.bin</code> file to flash, just select your
          radiosonde type below, then skip straight to
          <button
            type="button"
            class="font-medium text-blue-300 underline underline-offset-2 hover:text-blue-200 disabled:opacity-50 disabled:no-underline disabled:cursor-not-allowed"
            :disabled="!ui.hardwareSelected"
            @click="ui.goTo('flash')"
          >the Flash step</button>.
          The Configure and Build steps are only needed to produce a new binary.
        </p>
      </div>
    </div>

    <!-- Hardware cards -->
    <div class="grid grid-cols-1 lg:grid-cols-3 gap-4">
      <button
        v-for="hw in HARDWARE_OPTIONS"
        :key="hw.variant"
        type="button"
        class="text-left rounded-lg border-2 p-5 transition-colors focus:outline-none focus:ring-2 focus:ring-green-500 focus:ring-offset-2 focus:ring-offset-gray-950"
        :class="
          ui.hardwareVariant === hw.variant
            ? 'border-green-500 bg-green-900/15'
            : 'border-gray-700 bg-gray-900 hover:border-gray-500'
        "
        @click="selectHardware(hw.variant)"
      >
        <div class="flex items-start justify-between gap-2">
          <h2 class="text-base font-semibold text-gray-100">{{ hw.name }}</h2>
          <div
            v-if="ui.hardwareVariant === hw.variant"
            class="w-5 h-5 rounded-full bg-green-500 flex items-center justify-center shrink-0 mt-0.5"
          >
            <svg class="w-3 h-3 text-white" viewBox="0 0 12 12" fill="none" stroke="currentColor" stroke-width="2">
              <path d="M2 6l3 3 5-5" />
            </svg>
          </div>
        </div>

        <!-- Badge -->
        <span
          class="inline-block mt-2 px-2 py-0.5 text-xs font-medium rounded-full border"
          :class="BADGE_CLASSES[hw.badge.color]"
        >
          {{ hw.badge.label }}
        </span>

        <!-- Hardware identification image -->
        <div class="mt-3 rounded-md overflow-hidden border border-gray-800 bg-gray-950/50">
          <div
            v-if="hardwareImage(hw.variant)"
            class="relative cursor-zoom-in"
            @click.stop="openLightbox(hw.variant, hw.name)"
          >
            <img
              :src="hardwareImage(hw.variant)!"
              :alt="`${hw.name} board identification`"
              class="block w-full h-32 object-cover object-center bg-gray-950"
              loading="lazy"
            />
            <span class="absolute bottom-1 right-1 flex items-center gap-1 rounded bg-black/60 px-1.5 py-0.5 text-[10px] text-gray-100">
              <svg class="w-3 h-3" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="2">
                <circle cx="11" cy="11" r="7" />
                <path d="M21 21l-4.3-4.3" stroke-linecap="round" />
              </svg>
              Enlarge
            </span>
          </div>
          <div v-else class="p-6 text-center">
            <svg class="mx-auto w-10 h-10 text-gray-700 mb-2" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="1">
              <path d="M4 16l4.586-4.586a2 2 0 012.828 0L16 16m-2-2l1.586-1.586a2 2 0 012.828 0L20 14m-6-6h.01M6 20h12a2 2 0 002-2V6a2 2 0 00-2-2H6a2 2 0 00-2 2v12a2 2 0 002 2z" />
            </svg>
            <p class="text-xs text-gray-500">{{ hw.imagePlaceholder }}</p>
          </div>
        </div>

        <!-- Identification guide: PCB marking / serial / production (right below the photo) -->
        <div class="mt-3 space-y-1 text-xs text-gray-500">
          <p>{{ hw.identification }}</p>
          <p v-if="hw.serialPrefixes">Serial: starts with {{ hw.serialPrefixes }}</p>
          <p>Production: {{ hw.era }}</p>
        </div>

        <!-- Separator -->
        <hr class="my-3 border-gray-800" />

        <!-- Specs table (MCU / hardware info) -->
        <dl class="space-y-1">
          <div v-for="spec in hw.specs" :key="spec.label" class="flex gap-2 text-xs">
            <dt class="text-gray-500 w-12 shrink-0 font-medium">{{ spec.label }}</dt>
            <dd class="text-gray-300">{{ spec.value }}</dd>
          </div>
        </dl>

        <!-- Feature bullets -->
        <ul class="mt-3 space-y-1 text-sm text-gray-400">
          <li v-for="(feat, i) in hw.features" :key="i" class="flex items-start gap-2">
            <span class="text-gray-600 mt-0.5 shrink-0">&bull;</span>
            <span>{{ feat }}</span>
          </li>
        </ul>
      </button>
    </div>

    <!-- Learn more (collapsible) -->
    <details class="group">
      <summary class="cursor-pointer select-none text-xs text-gray-500 hover:text-gray-400 flex items-center gap-1.5">
        <svg
          class="w-3.5 h-3.5 transition-transform group-open:rotate-90"
          fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="2"
        >
          <path d="M9 5l7 7-7 7" />
        </svg>
        Learn more about RS41 model naming and hardware differences
      </summary>
      <div class="mt-4 space-y-4 text-sm text-gray-400 pl-5">
        <div>
          <h3 class="text-xs font-semibold text-gray-300 uppercase tracking-wider mb-1">Model naming convention</h3>
          <p>
            RS41 PCB models follow the format <strong class="text-gray-300">RSM4XY</strong>:
            X=1 (RS41-SG, no pressure sensor) or X=2 (RS41-SGP, with pressure sensor).
            Y = hardware revision (1&ndash;5).
            So RSM412 = SG revision 2, RSM424 = SGP revision 4.
          </p>
        </div>

        <div>
          <h3 class="text-xs font-semibold text-gray-300 uppercase tracking-wider mb-1">Hardware comparison</h3>
          <div class="overflow-x-auto">
            <table class="text-xs w-full">
              <thead>
                <tr class="text-left text-gray-500 border-b border-gray-800">
                  <th class="py-1.5 pr-4"></th>
                  <th class="py-1.5 pr-4 font-medium">Rev 1&ndash;2 (RSM4x1/4x2)</th>
                  <th class="py-1.5 font-medium">Rev 4&ndash;5 (RSM4x4/4x5)</th>
                </tr>
              </thead>
              <tbody class="text-gray-400">
                <tr class="border-b border-gray-800/50">
                  <td class="py-1.5 pr-4 text-gray-500 font-medium">MCU</td>
                  <td class="py-1.5 pr-4">STM32F100C8 (Cortex-M3, 64KB)</td>
                  <td class="py-1.5">STM32L412KB (Cortex-M4, 128KB)</td>
                </tr>
                <tr class="border-b border-gray-800/50">
                  <td class="py-1.5 pr-4 text-gray-500 font-medium">GPS</td>
                  <td class="py-1.5 pr-4">u-blox 6 (UBX-G6010)</td>
                  <td class="py-1.5">u-blox M10 (multi-GNSS)</td>
                </tr>
                <tr class="border-b border-gray-800/50">
                  <td class="py-1.5 pr-4 text-gray-500 font-medium">Radio</td>
                  <td class="py-1.5 pr-4">Si4032</td>
                  <td class="py-1.5">Si4032</td>
                </tr>
                <tr class="border-b border-gray-800/50">
                  <td class="py-1.5 pr-4 text-gray-500 font-medium">GPS config</td>
                  <td class="py-1.5 pr-4">Legacy UBX-CFG</td>
                  <td class="py-1.5">VALSET/VALGET</td>
                </tr>
                <tr class="border-b border-gray-800/50">
                  <td class="py-1.5 pr-4 text-gray-500 font-medium">Serial</td>
                  <td class="py-1.5 pr-4">S, P, R, T, V</td>
                  <td class="py-1.5">W (rev4), X (rev5)</td>
                </tr>
                <tr>
                  <td class="py-1.5 pr-4 text-gray-500 font-medium">Era</td>
                  <td class="py-1.5 pr-4">~2014&ndash;2023</td>
                  <td class="py-1.5">~2023+</td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>

        <div>
          <h3 class="text-xs font-semibold text-gray-300 uppercase tracking-wider mb-1">Notes</h3>
          <ul class="space-y-1 text-xs">
            <li>&bull; Revision 3 (RSM4x3) has never been observed in the wild</li>
            <li>&bull; RSM4x5 (X-prefix serials) introduced multi-GNSS telemetry in Vaisala firmware</li>
            <li>&bull; Pin-compatible MCU packages but different pin assignments &mdash; old firmware cannot run on new boards</li>
          </ul>
        </div>
      </div>
    </details>

    <!-- Navigation -->
    <div class="flex justify-end pt-4 border-t border-gray-800">
      <button
        type="button"
        class="px-4 py-2 text-sm font-medium bg-green-600 hover:bg-green-500 text-white rounded-md transition-colors disabled:opacity-50 disabled:cursor-not-allowed"
        :disabled="!ui.hardwareSelected"
        @click="ui.goNext()"
      >
        Next: Configure &rarr;
      </button>
    </div>

    <!-- Image lightbox -->
    <Teleport to="body">
      <div
        v-if="lightbox"
        class="fixed inset-0 z-50 flex items-center justify-center bg-black/80 p-4 cursor-zoom-out"
        @click="closeLightbox"
      >
        <button
          type="button"
          class="absolute top-4 right-4 text-gray-300 hover:text-white"
          aria-label="Close"
          @click="closeLightbox"
        >
          <svg class="w-7 h-7" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="2">
            <path d="M6 6l12 12M18 6L6 18" stroke-linecap="round" />
          </svg>
        </button>
        <img
          :src="lightbox.src"
          :alt="lightbox.alt"
          class="max-w-full max-h-full object-contain rounded-md shadow-2xl"
          @click.stop
        />
      </div>
    </Teleport>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onBeforeUnmount } from "vue";
import {
  useUiStore,
  variantToHardwareType,
  type HardwareVariant,
} from "@/stores/ui";
import { useConfigStore } from "@/stores/config";

const ui = useUiStore();
const configStore = useConfigStore();

// Hardware identification photos live in src/assets/hardware/, named by variant
// (e.g. RS41.png, RS41_RSM4x4.png, DFM17.png). Loaded via glob so the build does
// not break before the files are added — a missing image falls back to a placeholder.
const HARDWARE_IMAGES: Record<string, string> = Object.fromEntries(
  Object.entries(
    import.meta.glob("../../assets/hardware/*.{png,jpg,jpeg,webp}", {
      eager: true,
      query: "?url",
      import: "default",
    })
  ).map(([path, url]) => [
    path.split("/").pop()!.replace(/\.[^.]+$/, ""),
    url as string,
  ])
);

function hardwareImage(variant: HardwareVariant): string | null {
  return HARDWARE_IMAGES[variant] ?? null;
}

/** Full-resolution variant (e.g. RS41-large.jpg), falling back to the small image. */
function largeImage(variant: HardwareVariant): string | null {
  return HARDWARE_IMAGES[`${variant}-large`] ?? hardwareImage(variant);
}

// Lightbox state — the large image is only fetched by the browser when opened.
const lightbox = ref<{ src: string; alt: string } | null>(null);

function openLightbox(variant: HardwareVariant, name: string) {
  const src = largeImage(variant);
  if (src) lightbox.value = { src, alt: `${name} board identification` };
}

function closeLightbox() {
  lightbox.value = null;
}

function onKeydown(e: KeyboardEvent) {
  if (e.key === "Escape") closeLightbox();
}

onMounted(() => window.addEventListener("keydown", onKeydown));
onBeforeUnmount(() => window.removeEventListener("keydown", onKeydown));

interface HardwareOption {
  variant: HardwareVariant;
  name: string;
  badge: { label: string; color: "green" | "blue" | "yellow" | "amber" };
  specs: { label: string; value: string }[];
  features: string[];
  serialPrefixes: string;
  era: string;
  identification: string;
  imagePlaceholder: string;
}

const BADGE_CLASSES: Record<string, string> = {
  green: "bg-green-900/20 text-green-400 border-green-700/30",
  blue: "bg-blue-900/20 text-blue-400 border-blue-700/30",
  yellow: "bg-yellow-900/20 text-yellow-400 border-yellow-700/30",
  amber: "bg-amber-900/20 text-amber-400 border-amber-700/30",
};

const HARDWARE_OPTIONS: HardwareOption[] = [
  {
    variant: "RS41",
    name: "RS41 — RSM4x1 / RSM4x2",
    badge: { label: "Stable", color: "green" },
    specs: [
      { label: "MCU", value: "STM32F100 (Cortex-M3, 64KB)" },
      { label: "GPS", value: "u-blox 6 (UBX-G6010)" },
      { label: "Radio", value: "Si4032 (70 cm)" },
    ],
    features: [
      "External oscillator (no calibration needed)",
      "External I2C bus for sensors and Si5351",
      "10-pin programming connector",
    ],
    serialPrefixes: "S, P, R, T, or V",
    era: "~2014\u20132023",
    identification:
      'PCB marked "RSM411"\u2013"RSM422" near the NFC antenna / ON-OFF button.',
    imagePlaceholder:
      "Photo of RS41 RSM4x1/RSM4x2 board showing revision marking",
  },
  {
    variant: "RS41_RSM4x4",
    name: "RS41 — RSM4x4 / RSM4x5",
    badge: { label: "New", color: "blue" },
    specs: [
      { label: "MCU", value: "STM32L412 (Cortex-M4, 128KB)" },
      { label: "GPS", value: "u-blox M10 (multi-GNSS)" },
      { label: "Radio", value: "Si4032 (70 cm)" },
    ],
    features: [
      "External oscillator (no calibration needed)",
      "External I2C bus for sensors and Si5351",
      "10-pin programming connector",
    ],
    serialPrefixes: "W (rev 4) or X (rev 5)",
    era: "~2023+",
    identification:
      'PCB marked "RSM414" or higher near the NFC antenna / ON-OFF button.',
    imagePlaceholder:
      "Photo of RS41 RSM4x4/RSM4x5 board showing revision marking",
  },
  {
    variant: "DFM17",
    name: "DFM-17",
    badge: { label: "Experimental", color: "yellow" },
    specs: [
      { label: "MCU", value: "STM32F100" },
      { label: "GPS", value: "Internal" },
      { label: "Radio", value: "Si4063 (70 cm)" },
    ],
    features: [
      "Internal oscillator (requires GPS lock to calibrate)",
      "No external I2C bus or Si5351 support",
      "Programming via unpopulated PCB pads (soldering required)",
    ],
    serialPrefixes: "",
    era: "Current production",
    identification:
      "Graw DFM-17 sonde. Programming connector is unpopulated pads between the sensor boom connector and the STM32.",
    imagePlaceholder:
      "Photo of DFM-17 board showing programming pad location",
  },
];

function selectHardware(variant: HardwareVariant) {
  ui.hardwareVariant = variant;
  configStore.set("hardware", "type", variantToHardwareType(variant));
}
</script>
