<template>
  <div
    v-if="conflicts.length > 0"
    class="rounded-md bg-red-900/30 border border-red-700/50 p-4 space-y-1"
    role="alert"
  >
    <p class="text-xs font-semibold text-red-400 uppercase tracking-wider">
      Configuration conflicts
    </p>
    <ul class="mt-2 space-y-1">
      <li v-for="(msg, i) in conflicts" :key="i" class="flex items-start gap-2 text-sm text-red-300">
        <span class="mt-0.5 shrink-0">⚠</span>
        <span>{{ msg }}</span>
      </li>
    </ul>
  </div>
</template>

<script setup lang="ts">
import { computed } from "vue";
import type { Schema } from "@/types/schema";
import { detectConflicts } from "@/config/validation";
import { useConfigStore } from "@/stores/config";

const props = defineProps<{
  schema: Schema;
}>();

const configStore = useConfigStore();

const conflicts = computed(() => detectConflicts(props.schema, configStore.values));
</script>
