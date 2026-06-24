<template>
  <div class="space-y-3">
    <div class="flex items-center gap-0">
      <div
        v-for="(step, i) in FLASH_STEPS"
        :key="step.id"
        class="flex items-center"
        :class="i < FLASH_STEPS.length - 1 ? 'flex-1' : ''"
      >
        <div class="flex items-center gap-1.5 shrink-0">
          <div
            class="w-2 h-2 rounded-full transition-colors"
            :class="getStepColor(step.id)"
          />
          <span class="text-xs hidden sm:block" :class="getStepTextColor(step.id)">
            {{ step.label }}
          </span>
        </div>
        <div v-if="i < FLASH_STEPS.length - 1" class="flex-1 h-px bg-gray-800 mx-2" />
      </div>
    </div>

    <!-- Erase / program: real % from ST-LINK bargraph callbacks -->
    <div v-if="flasher.status === 'erasing' || flasher.status === 'flashing'" class="space-y-1">
      <div class="flex justify-between text-xs text-gray-500">
        <span>{{ flasher.status === "erasing" ? "Erasing" : "Writing" }}</span>
        <span>{{ flasher.progress }}%</span>
      </div>
      <div class="h-1.5 rounded-full bg-gray-800 overflow-hidden">
        <div
          class="h-full rounded-full bg-green-500 transition-all duration-150"
          :style="{ width: `${flasher.progress}%` }"
        />
      </div>
    </div>

    <!-- Real progress bar for verifying (chunked read-back) -->
    <div v-if="flasher.status === 'verifying'" class="space-y-1">
      <div class="flex justify-between text-xs text-gray-500">
        <span>Verifying</span>
        <span>{{ flasher.progress }}%</span>
      </div>
      <div class="h-1.5 rounded-full bg-gray-800 overflow-hidden">
        <div
          class="h-full rounded-full bg-green-500 transition-all"
          :style="{ width: `${flasher.progress}%` }"
        />
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, watch } from "vue";
import { useFlasherStore } from "@/stores/flasher";
import type { FlashStatus } from "@/config/flash-strategy";

const flasher = useFlasherStore();

/** Track the last active (non-error, non-idle) status so we know which step failed */
const failedAtStatus = ref<FlashStatus | null>(null);
watch(
  () => flasher.status,
  (cur, prev) => {
    if (cur === "error" && prev && prev !== "error" && prev !== "idle") {
      failedAtStatus.value = prev;
    } else if (cur === "idle") {
      failedAtStatus.value = null;
    }
  }
);

const FLASH_STEPS: { id: FlashStatus; label: string }[] = [
  { id: "connecting", label: "Connect" },
  { id: "connected", label: "Identify" },
  { id: "erasing", label: "Erase" },
  { id: "flashing", label: "Write" },
  { id: "verifying", label: "Verify" },
  { id: "done", label: "Done" },
];

const STATUS_ORDER: Partial<Record<FlashStatus, number>> = {
  connecting: 0,
  connected: 1,
  erasing: 2,
  flashing: 3,
  verifying: 4,
  done: 5,
};

function getStepColor(stepId: FlashStatus): string {
  if (flasher.status === "error") {
    const failedAt = STATUS_ORDER[failedAtStatus.value!] ?? -1;
    const step = STATUS_ORDER[stepId] ?? 99;
    if (step === failedAt) return "bg-red-500";
    return step < failedAt ? "bg-green-700" : "bg-gray-700";
  }
  if (flasher.status === stepId) return "bg-green-400 animate-pulse";
  const current = STATUS_ORDER[flasher.status] ?? -1;
  const step = STATUS_ORDER[stepId] ?? 99;
  return step < current ? "bg-green-700" : "bg-gray-700";
}

function getStepTextColor(stepId: FlashStatus): string {
  if (flasher.status === "error") {
    const failedAt = STATUS_ORDER[failedAtStatus.value!] ?? -1;
    const step = STATUS_ORDER[stepId] ?? 99;
    if (step === failedAt) return "text-red-400";
    return step < failedAt ? "text-gray-400" : "text-gray-600";
  }
  const current = STATUS_ORDER[flasher.status] ?? -1;
  const step = STATUS_ORDER[stepId] ?? 99;
  if (flasher.status === stepId) return "text-green-400";
  return step < current ? "text-gray-400" : "text-gray-600";
}
</script>

