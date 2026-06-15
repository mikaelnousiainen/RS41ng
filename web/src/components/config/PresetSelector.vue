<template>
  <div class="space-y-3">
    <div class="flex items-center justify-between">
      <span class="text-xs text-gray-500 uppercase tracking-wider">Quick start preset</span>
      <button
        v-if="activePresetId"
        type="button"
        class="text-xs text-gray-600 hover:text-gray-400 transition-colors"
        @click="clearPreset"
      >
        Clear
      </button>
    </div>

    <div class="grid grid-cols-1 sm:grid-cols-2 gap-2">
      <button
        v-for="preset in filteredPresets"
        :key="preset.id"
        type="button"
        class="text-left p-3 rounded-md border transition-colors"
        :class="
          activePresetId === preset.id
            ? 'border-green-500 bg-green-900/20 text-gray-100'
            : 'border-gray-700 bg-gray-800/50 text-gray-300 hover:border-gray-600 hover:text-gray-100 hover:bg-gray-800'
        "
        @click="apply(preset)"
      >
        <div class="text-sm font-medium">{{ preset.label }}</div>
        <div class="text-xs text-gray-500 mt-0.5 leading-snug">{{ preset.description }}</div>
      </button>
    </div>

    <p class="text-xs text-gray-600">
      Applies default values for the selected mode. You can still change individual settings
      below.
    </p>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from "vue";
import { PRESETS, type Preset } from "@/config/presets";
import { useConfigStore } from "@/stores/config";

const configStore = useConfigStore();
const activePresetId = ref<string | null>(null);

const currentHardware = computed(
  () => (configStore.get("hardware", "type") as string | undefined) ?? "RS41"
);

const filteredPresets = computed(() =>
  PRESETS.filter((p: Preset) =>
    p.hardware.includes(currentHardware.value as Preset["hardware"][number])
  )
);

function apply(preset: Preset) {
  activePresetId.value = preset.id;
  configStore.applyPreset(preset.overlay);
}

function clearPreset() {
  activePresetId.value = null;
}
</script>
