import { describe, it, expect, beforeEach } from "vitest";
import { mount } from "@vue/test-utils";
import { createPinia, setActivePinia } from "pinia";
import PresetSelector from "@/components/config/PresetSelector.vue";
import { useConfigStore } from "@/stores/config";

describe("PresetSelector", () => {
  beforeEach(() => {
    setActivePinia(createPinia());
  });

  it("renders preset buttons", () => {
    const wrapper = mount(PresetSelector);
    // There are RS41 presets visible by default (hardware defaults to RS41)
    const buttons = wrapper.findAll("button").filter((b) => b.text().length > 5);
    expect(buttons.length).toBeGreaterThan(0);
  });

  it("shows only RS41 presets when hardware is RS41 (default)", () => {
    const wrapper = mount(PresetSelector);
    const text = wrapper.text();
    expect(text).toContain("RS41");
    expect(text).not.toContain("DFM-17");
  });

  it("shows only DFM-17 presets when hardware.type is DFM17", () => {
    const pinia = createPinia();
    setActivePinia(pinia);
    const configStore = useConfigStore();
    configStore.set("hardware", "type", "DFM17");

    const wrapper = mount(PresetSelector);
    const text = wrapper.text();
    expect(text).toContain("DFM-17");
    expect(text).not.toContain("Horus V2"); // RS41-only preset
  });

  it("applies preset overlay to config store on click", async () => {
    const pinia = createPinia();
    setActivePinia(pinia);
    const configStore = useConfigStore();

    const wrapper = mount(PresetSelector);

    // Find the APRS RS41 preset button and click it
    const aprsBtn = wrapper
      .findAll("button")
      .find((b) => b.text().includes("APRS only"));
    expect(aprsBtn).toBeDefined();
    await aprsBtn!.trigger("click");

    // After applying APRS preset, tx_aprs should be true in tx_modes
    expect(configStore.get("tx_modes", "radio_tx_aprs")).toBe(true);
  });

  it("highlights the active preset after clicking", async () => {
    const wrapper = mount(PresetSelector);
    const buttons = wrapper.findAll("button").filter((b) => b.text().includes("APRS only"));
    await buttons[0].trigger("click");

    // Active preset button should have a green border class
    expect(buttons[0].classes().some((c) => c.includes("green"))).toBe(true);
  });
});
