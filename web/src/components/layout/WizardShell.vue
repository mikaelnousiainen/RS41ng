<template>
  <div class="min-h-screen flex flex-col bg-gray-950">
    <!-- Header -->
    <header class="border-b border-gray-800 px-6 py-4 flex-none">
      <div class="max-w-5xl mx-auto flex items-center gap-3">
        <span class="font-mono font-bold text-green-400 text-lg tracking-tight">
          RS41ng
        </span>
        <span class="text-gray-600">/</span>
        <span class="text-gray-400 text-sm">Firmware Configurator</span>

        <!-- Selected hardware indicator -->
        <span
          class="ml-auto inline-flex items-center gap-1.5 px-2.5 py-1 text-xs font-medium rounded-full border"
          :class="ui.hardwareSelected
            ? 'border-gray-700 bg-gray-800/80 text-gray-300'
            : 'border-gray-800 bg-transparent text-gray-600 italic'"
        >
          <span v-if="ui.hardwareSelected" class="w-1.5 h-1.5 rounded-full bg-green-500" />
          {{ ui.hardwareVariant ? HARDWARE_LABELS[ui.hardwareVariant] : '[select sonde type]' }}
        </span>
      </div>
    </header>

    <!-- Step indicator -->
    <nav class="border-b border-gray-800 px-6 py-0 flex-none">
      <div class="max-w-5xl mx-auto flex items-center">
        <template v-for="(step, index) in STEPS" :key="step">
          <!-- Step button -->
          <button
            type="button"
            class="flex items-center gap-2.5 px-4 py-4 text-sm font-medium transition-colors focus:outline-none"
            :class="stepButtonClass(step)"
            @click="ui.goTo(step)"
          >
            <span
              class="w-6 h-6 rounded-full flex items-center justify-center text-xs font-mono font-bold flex-none"
              :class="stepCircleClass(step)"
            >
              {{ index + 1 }}
            </span>
            <span>{{ STEP_LABELS[step] }}</span>
          </button>

          <!-- Connector line -->
          <div
            v-if="index < STEPS.length - 1"
            class="flex-1 h-px mx-1"
            :class="index < ui.stepIndex ? 'bg-gray-600' : 'bg-gray-800'"
          />
        </template>
      </div>
    </nav>

    <!-- Step content -->
    <main class="flex-1 overflow-y-auto">
      <div class="max-w-5xl mx-auto px-6 py-8">
        <slot />
      </div>
    </main>
  </div>
</template>

<script setup lang="ts">
import { useUiStore, STEPS, STEP_LABELS, type Step } from "@/stores/ui";

const ui = useUiStore();

const HARDWARE_LABELS: Record<string, string> = {
  RS41: "RS41 (RSM4x1/RSM4x2)",
  RS41_RSM4x4: "RS41 (RSM4x4/RSM4x5)",
  DFM17: "DFM-17",
};

/** Steps beyond "hardware" are locked until a hardware variant is selected */
function isStepLocked(step: Step): boolean {
  return step !== "hardware" && !ui.hardwareSelected;
}

function stepButtonClass(step: Step): string {
  if (isStepLocked(step))
    return "text-gray-700 cursor-not-allowed border-b-2 border-transparent -mb-px";
  if (step === ui.currentStep)
    return "text-white border-b-2 border-green-400 -mb-px";
  return "text-gray-500 hover:text-gray-300 border-b-2 border-transparent -mb-px";
}

function stepCircleClass(step: Step): string {
  if (isStepLocked(step)) return "bg-gray-800 text-gray-700";
  const idx = STEPS.indexOf(step);
  if (step === ui.currentStep) return "bg-green-500 text-white";
  if (idx < ui.stepIndex) return "bg-gray-600 text-gray-200";
  return "bg-gray-800 text-gray-500";
}
</script>
