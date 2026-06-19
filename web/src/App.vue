<template>
  <WizardShell>
    <HardwareStep v-if="ui.currentStep === 'hardware'" />
    <ConfigureStep v-else-if="ui.currentStep === 'configure'" />
    <BuildStep v-else-if="ui.currentStep === 'build'" />
    <FlashStep v-else-if="ui.currentStep === 'flash'" />
  </WizardShell>
</template>

<script setup lang="ts">
import { onMounted } from "vue";
import WizardShell from "@/components/layout/WizardShell.vue";
import HardwareStep from "@/components/steps/HardwareStep.vue";
import ConfigureStep from "@/components/steps/ConfigureStep.vue";
import BuildStep from "@/components/steps/BuildStep.vue";
import FlashStep from "@/components/steps/FlashStep.vue";
import { useUiStore } from "@/stores/ui";
import { useConfigStore } from "@/stores/config";
import { schema } from "@/config/schema-loader";
import { buildDefaults } from "@/config/defaults";

const ui = useUiStore();
const configStore = useConfigStore();

onMounted(() => {
  ui.initOS();
  configStore.reset(buildDefaults(schema));
});
</script>
