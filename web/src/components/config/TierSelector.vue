<template>
  <div class="flex items-center gap-3">
    <span class="text-xs text-gray-500 uppercase tracking-wider shrink-0">Detail level:</span>
    <div class="flex rounded-md overflow-hidden border border-gray-700">
      <button
        v-for="t in TIERS"
        :key="t.value"
        type="button"
        class="px-3 py-1.5 text-xs font-medium transition-colors"
        :class="
          ui.tier === t.value
            ? 'bg-green-600 text-white'
            : 'bg-gray-800 text-gray-400 hover:text-gray-200'
        "
        @click="ui.tier = t.value"
      >
        {{ t.label }}
      </button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { useUiStore, type Tier } from "@/stores/ui";

const ui = useUiStore();

const TIERS: { value: Tier; label: string }[] = [
  { value: "simple", label: "Simple" },
  { value: "intermediate", label: "Intermediate" },
  { value: "advanced", label: "Advanced" },
];
</script>
