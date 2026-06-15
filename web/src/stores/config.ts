import { defineStore } from "pinia";
import { ref } from "vue";
import type { ConfigState, ConfigValue } from "@/types/config";

export const useConfigStore = defineStore("config", () => {
  const values = ref<ConfigState>({});

  function set(section: string, field: string, value: ConfigValue) {
    if (!values.value[section]) values.value[section] = {};
    values.value[section][field] = value;
  }

  function get(section: string, field: string): ConfigValue | undefined {
    return values.value[section]?.[field];
  }

  function reset(initial: ConfigState) {
    values.value = JSON.parse(JSON.stringify(initial)) as ConfigState;
  }

  function applyPreset(overlay: ConfigState) {
    for (const [section, fields] of Object.entries(overlay)) {
      if (!values.value[section]) values.value[section] = {};
      Object.assign(values.value[section], fields);
    }
  }

  return { values, set, get, reset, applyPreset };
});
