import { defineStore } from "pinia";
import { ref, computed } from "vue";
import type { McuFamily } from "@/config/flash-strategy";

export type Step = "hardware" | "configure" | "build" | "flash";
export type Tier = "simple" | "intermediate" | "advanced";
export type OS = "linux" | "macos" | "windows" | "unknown";
export type HardwareVariant = "RS41" | "RS41_RSM4x4" | "DFM17";

export const STEPS: Step[] = ["hardware", "configure", "build", "flash"];

export const STEP_LABELS: Record<Step, string> = {
  hardware: "Hardware",
  configure: "Configure",
  build: "Build",
  flash: "Flash",
};

/** Maps hardware variant to the schema hardware type value (they are now 1:1) */
export function variantToHardwareType(
  variant: HardwareVariant
): "RS41" | "RS41_RSM4x4" | "DFM17" {
  return variant;
}

/** Maps hardware variant to expected MCU family for flash mismatch detection */
export function variantToMcuFamily(variant: HardwareVariant): McuFamily {
  switch (variant) {
    case "RS41_RSM4x4":
      return "L4";
    case "RS41":
    case "DFM17":
    default:
      return "F1";
  }
}

function detectOS(): OS {
  const ua = navigator.userAgent.toLowerCase();
  if (ua.includes("linux")) return "linux";
  if (ua.includes("mac")) return "macos";
  if (ua.includes("win")) return "windows";
  return "unknown";
}

export const useUiStore = defineStore("ui", () => {
  const currentStep = ref<Step>("hardware");
  const tier = ref<Tier>("simple");
  const os = ref<OS>("unknown");
  const hardwareVariant = ref<HardwareVariant | null>(null);

  const stepIndex = computed(() => STEPS.indexOf(currentStep.value));
  const hardwareSelected = computed(() => hardwareVariant.value !== null);

  function scrollToTop() {
    window.scrollTo({ top: 0, behavior: "instant" });
  }

  function goTo(step: Step) {
    // Block navigation past hardware step until a variant is selected
    if (!hardwareSelected.value && step !== "hardware") return;
    currentStep.value = step;
    scrollToTop();
  }

  function goNext() {
    if (!hardwareSelected.value && currentStep.value === "hardware") return;
    const next = STEPS[stepIndex.value + 1];
    if (next) {
      currentStep.value = next;
      scrollToTop();
    }
  }

  function goBack() {
    const prev = STEPS[stepIndex.value - 1];
    if (prev) {
      currentStep.value = prev;
      scrollToTop();
    }
  }

  function initOS() {
    os.value = detectOS();
  }

  return {
    currentStep,
    tier,
    os,
    hardwareVariant,
    stepIndex,
    hardwareSelected,
    goTo,
    goNext,
    goBack,
    initOS,
  };
});
