<template>
  <div class="rounded-md bg-gray-950 border border-gray-800 overflow-hidden">
    <div v-if="label" class="flex items-center justify-between px-3 py-1.5 bg-gray-900 border-b border-gray-800">
      <span class="text-xs text-gray-500 font-mono">{{ label }}</span>
      <CopyButton :text="command" />
    </div>
    <pre class="px-4 py-3 text-sm text-green-300 font-mono overflow-x-auto whitespace-pre-wrap">{{ command }}</pre>
  </div>
</template>

<script setup lang="ts">
import CopyButton from "@/components/common/CopyButton.vue";

defineProps<{
  command: string;
  label?: string;
}>();
</script>
