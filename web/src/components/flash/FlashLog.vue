<template>
  <div
    v-if="flasher.log.length > 0"
    class="rounded-md bg-gray-950 border border-gray-800 overflow-hidden"
  >
    <div class="px-3 py-1.5 bg-gray-900 border-b border-gray-800">
      <span class="text-xs text-gray-500 font-mono">Flash log</span>
    </div>
    <div ref="logEl" class="h-40 overflow-y-auto px-3 py-2 space-y-0.5 font-mono text-xs">
      <div
        v-for="(entry, i) in flasher.log"
        :key="i"
        :class="
          entry.level === 'error'
            ? 'text-red-400'
            : entry.level === 'warn'
              ? 'text-yellow-400'
              : 'text-gray-400'
        "
      >
        {{ entry.message }}
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, nextTick } from "vue";
import { useFlasherStore } from "@/stores/flasher";

const flasher = useFlasherStore();
const logEl = ref<HTMLElement | null>(null);

watch(
  () => flasher.log.length,
  async () => {
    await nextTick();
    if (logEl.value) logEl.value.scrollTop = logEl.value.scrollHeight;
  }
);
</script>
