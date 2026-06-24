import { describe, it, expect, beforeEach } from "vitest";
import { mount } from "@vue/test-utils";
import { createPinia, setActivePinia } from "pinia";
import FieldRenderer from "@/components/config/FieldRenderer.vue";
import { useConfigStore } from "@/stores/config";
import type { SchemaField } from "@/types/schema";

function makeField(overrides: Partial<SchemaField> & { type: string }): SchemaField {
  return {
    define: "TEST_FIELD",
    label: "Test Field",
    tier: "simple",
    default: undefined,
    ...overrides,
  } as unknown as SchemaField;
}

describe("FieldRenderer", () => {
  beforeEach(() => {
    setActivePinia(createPinia());
  });

  it("renders a text input for string fields", () => {
    const wrapper = mount(FieldRenderer, {
      props: {
        sectionKey: "global",
        fieldKey: "callsign",
        field: makeField({ type: "string", define: "CALLSIGN", default: "N0CALL" }),
      },
    });
    expect(wrapper.find('input[type="text"]').exists()).toBe(true);
  });

  it("renders a toggle button for bool fields", () => {
    const wrapper = mount(FieldRenderer, {
      props: {
        sectionKey: "radio_si4032",
        fieldKey: "tx_aprs",
        field: makeField({ type: "bool", define: "TX_APRS", default: false }),
      },
    });
    expect(wrapper.find("button[role='switch']").exists()).toBe(true);
  });

  it("renders a number input for integer fields", () => {
    const wrapper = mount(FieldRenderer, {
      props: {
        sectionKey: "radio_si4032",
        fieldKey: "tx_power",
        field: makeField({ type: "integer", define: "TX_POWER", default: 7 }),
      },
    });
    expect(wrapper.find('input[type="number"]').exists()).toBe(true);
  });

  it("renders a select for enum fields", () => {
    const wrapper = mount(FieldRenderer, {
      props: {
        sectionKey: "hardware",
        fieldKey: "type",
        field: makeField({
          type: "enum",
          define: "HARDWARE_TYPE",
          default: "RS41",
          values: [
            { value: "RS41", label: "RS41" },
            { value: "DFM17", label: "DFM-17" },
          ],
        }),
      },
    });
    expect(wrapper.find("select").exists()).toBe(true);
  });

  it("reads current value from config store", () => {
    const pinia = createPinia();
    setActivePinia(pinia);
    const configStore = useConfigStore();
    configStore.set("global", "callsign", "OH2A");

    const wrapper = mount(FieldRenderer, {
      props: {
        sectionKey: "global",
        fieldKey: "callsign",
        field: makeField({ type: "string", define: "CALLSIGN", default: "N0CALL" }),
      },
    });
    const input = wrapper.find('input[type="text"]').element as HTMLInputElement;
    expect(input.value).toBe("OH2A");
  });

  it("writes value to config store on change", async () => {
    const pinia = createPinia();
    setActivePinia(pinia);
    const configStore = useConfigStore();

    const wrapper = mount(FieldRenderer, {
      props: {
        sectionKey: "global",
        fieldKey: "callsign",
        field: makeField({ type: "string", define: "CALLSIGN", default: "N0CALL" }),
      },
    });

    const input = wrapper.find('input[type="text"]');
    await input.setValue("OH2A");
    expect(configStore.get("global", "callsign")).toBe("OH2A");
  });

  it("renders nothing for unknown field types", () => {
    const wrapper = mount(FieldRenderer, {
      props: {
        sectionKey: "global",
        fieldKey: "mystery",
        field: makeField({ type: "unknown_type" as any, define: "MYSTERY" }),
      },
    });
    expect(wrapper.html()).toBe("<!--v-if-->");
  });
});
