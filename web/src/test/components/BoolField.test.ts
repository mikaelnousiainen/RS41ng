import { describe, it, expect } from "vitest";
import { mount } from "@vue/test-utils";
import BoolField from "@/components/config/fields/BoolField.vue";
import type { SchemaField } from "@/types/schema";

const field: SchemaField = {
  define: "LEDS_ENABLE",
  type: "bool",
  label: "Enable LEDs",
  tier: "simple",
  default: true,
};

describe("BoolField", () => {
  it("renders a toggle button", () => {
    const wrapper = mount(BoolField, {
      props: { field, modelValue: false, errors: [] },
    });
    expect(wrapper.find("button[role='switch']").exists()).toBe(true);
  });

  it("shows the field label", () => {
    const wrapper = mount(BoolField, {
      props: { field, modelValue: false, errors: [] },
    });
    expect(wrapper.text()).toContain("Enable LEDs");
  });

  it("reflects true state with correct aria-checked", () => {
    const wrapper = mount(BoolField, {
      props: { field, modelValue: true, errors: [] },
    });
    expect(wrapper.find("button[role='switch']").attributes("aria-checked")).toBe("true");
  });

  it("reflects false state with aria-checked false", () => {
    const wrapper = mount(BoolField, {
      props: { field, modelValue: false, errors: [] },
    });
    expect(wrapper.find("button[role='switch']").attributes("aria-checked")).toBe("false");
  });

  it("emits update:modelValue with toggled value when clicked", async () => {
    const wrapper = mount(BoolField, {
      props: { field, modelValue: false, errors: [] },
    });
    await wrapper.find("button[role='switch']").trigger("click");
    expect(wrapper.emitted("update:modelValue")?.[0]).toEqual([true]);
  });

  it("emits false when toggling from true", async () => {
    const wrapper = mount(BoolField, {
      props: { field, modelValue: true, errors: [] },
    });
    await wrapper.find("button[role='switch']").trigger("click");
    expect(wrapper.emitted("update:modelValue")?.[0]).toEqual([false]);
  });

  it("displays validation errors", () => {
    const wrapper = mount(BoolField, {
      props: { field, modelValue: undefined, errors: ["Required"] },
    });
    expect(wrapper.text()).toContain("Required");
  });
});
