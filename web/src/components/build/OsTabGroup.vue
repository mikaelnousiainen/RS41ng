<template>
  <div class="space-y-4">
    <!-- OS tab bar -->
    <div class="flex border-b border-gray-800">
      <button
        v-for="tab in OS_TABS"
        :key="tab.os"
        type="button"
        class="px-4 py-2 text-sm font-medium transition-colors border-b-2 -mb-px"
        :class="
          activeOs === tab.os
            ? 'border-green-500 text-green-400'
            : 'border-transparent text-gray-500 hover:text-gray-300'
        "
        @click="activeOs = tab.os"
      >
        {{ tab.label }}
      </button>
    </div>

    <!-- Tab content slot -->
    <slot :os="activeOs" />
  </div>
</template>

<script setup lang="ts">
import { ref, watch } from "vue";
import type { OS } from "@/stores/ui";
import { useUiStore } from "@/stores/ui";

const OS_TABS: { os: OS; label: string }[] = [
  { os: "linux", label: "Linux" },
  { os: "macos", label: "macOS" },
  { os: "windows", label: "Windows" },
];

const ui = useUiStore();
const activeOs = ref<OS>(ui.os === "unknown" ? "linux" : ui.os);

// If the store OS is resolved after mount, sync the tab
watch(
  () => ui.os,
  (newOs) => {
    if (newOs !== "unknown") activeOs.value = newOs;
  }
);
</script>
