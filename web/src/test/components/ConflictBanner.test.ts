import { describe, it, expect, beforeEach } from "vitest";
import { mount } from "@vue/test-utils";
import { createPinia, setActivePinia } from "pinia";
import ConflictBanner from "@/components/config/ConflictBanner.vue";
import { useConfigStore } from "@/stores/config";
import type { Schema } from "@/types/schema";

function makeSchema(): Schema {
  return {
    conflicts: [
      {
        id: "nmea_i2c",
        message: "GPS NMEA output conflicts with I2C sensors.",
        condition: "gps.nmea_enable && sensors.bmp280_enable",
      },
    ],
    gps: {
      _section: "GPS",
      _tier: "advanced",
      _order: 0,
      nmea_enable: {
        define: "GPS_NMEA",
        type: "bool",
        label: "NMEA Output",
        tier: "advanced",
        default: false,
      },
    },
    sensors: {
      _section: "Sensors",
      _tier: "advanced",
      _order: 1,
      bmp280_enable: {
        define: "BMP280",
        type: "bool",
        label: "BMP280",
        tier: "advanced",
        default: false,
      },
    },
  } as unknown as Schema;
}

describe("ConflictBanner", () => {
  beforeEach(() => {
    setActivePinia(createPinia());
  });

  it("is hidden when there are no conflicts", () => {
    const wrapper = mount(ConflictBanner, {
      props: { schema: makeSchema() },
    });
    expect(wrapper.find("[role='alert']").exists()).toBe(false);
  });

  it("shows the conflict banner when a conflict is triggered", async () => {
    const pinia = createPinia();
    setActivePinia(pinia);
    const configStore = useConfigStore();
    configStore.set("gps", "nmea_enable", true);
    configStore.set("sensors", "bmp280_enable", true);

    const wrapper = mount(ConflictBanner, {
      props: { schema: makeSchema() },
    });
    expect(wrapper.find("[role='alert']").exists()).toBe(true);
    expect(wrapper.text()).toContain("GPS NMEA output conflicts with I2C sensors.");
  });

  it("hides the banner after conflict is resolved", async () => {
    const pinia = createPinia();
    setActivePinia(pinia);
    const configStore = useConfigStore();
    configStore.set("gps", "nmea_enable", true);
    configStore.set("sensors", "bmp280_enable", true);

    const wrapper = mount(ConflictBanner, {
      props: { schema: makeSchema() },
    });
    expect(wrapper.find("[role='alert']").exists()).toBe(true);

    configStore.set("sensors", "bmp280_enable", false);
    await wrapper.vm.$nextTick();
    expect(wrapper.find("[role='alert']").exists()).toBe(false);
  });
});
