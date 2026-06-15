import { describe, it, expect } from "vitest";
import type { Schema } from "@/types/schema";
import type { ConfigState } from "@/types/config";
import { generateConfigC } from "@/config/generate-source";

function makeMessagesSchema(): Schema {
  return {
    cw: {
      _section: "CW / Morse",
      _tier: "intermediate",
      _order: 10,
      messages: {
        define: "cw_message_templates",
        type: "string_array",
        label: "CW Messages",
        tier: "intermediate",
        file: "config.c",
        default: ["$cs $loc6 $altm"],
        validation: { max_item_length: 64 },
      },
    },
    pip: {
      _section: "Pip",
      _tier: "intermediate",
      _order: 11,
      messages: {
        define: "pip_message_templates",
        type: "string_array",
        label: "PIP Messages",
        tier: "intermediate",
        file: "config.c",
        default: ["$cs"],
        validation: { max_item_length: 64 },
      },
    },
    aprs: {
      _section: "APRS",
      _tier: "simple",
      _order: 5,
      messages: {
        define: "aprs_comment_templates",
        type: "string_array",
        label: "APRS Comment",
        tier: "intermediate",
        file: "config.c",
        default: ["RS41ng"],
        validation: { max_item_length: 43 },
      },
    },
    cats: {
      _section: "CATS",
      _tier: "simple",
      _order: 8,
      messages: {
        define: "cats_comment_templates",
        type: "string_array",
        label: "CATS Comment",
        tier: "intermediate",
        file: "config.c",
        default: ["RS41ng"],
        validation: { max_item_length: 100 },
      },
    },
    fsq: {
      _section: "FSQ",
      _tier: "advanced",
      _order: 15,
      messages: {
        define: "fsq_comment_templates",
        type: "string_array",
        label: "FSQ Comment",
        tier: "advanced",
        file: "config.c",
        default: ["RS41ng"],
        validation: { max_item_length: 100 },
      },
    },
    ft8: {
      _section: "FT8 / JT Digital Modes",
      _tier: "advanced",
      _order: 16,
      messages: {
        define: "ftjt_message_templates",
        type: "string_array",
        label: "FT8/JT65 Messages",
        tier: "advanced",
        file: "config.c",
        default: ["CQ $cs $loc4"],
        validation: { max_item_length: 13 },
      },
    },
  } as unknown as Schema;
}

describe("generateConfigC — static header lines", () => {
  it("includes template variable comment block", () => {
    const out = generateConfigC(makeMessagesSchema(), {});
    expect(out).toContain("$cs - Call sign");
    expect(out).toContain("$loc6 - Locator (6 chars)");
    expect(out).toContain("$alt - Altitude in meters");
  });

  it("includes stdlib and config.h includes", () => {
    const out = generateConfigC(makeMessagesSchema(), {});
    expect(out).toContain('#include <stdlib.h>');
    expect(out).toContain('#include "config.h"');
  });

  it("emits bool enable variable declarations", () => {
    const out = generateConfigC(makeMessagesSchema(), {});
    expect(out).toContain("bool leds_enabled = LEDS_ENABLE;");
    expect(out).toContain("bool bmp280_enabled = SENSOR_BMP280_ENABLE;");
    expect(out).toContain("bool si5351_enabled = RADIO_SI5351_ENABLE;");
    expect(out).toContain("bool gps_nmea_output_enabled = GPS_NMEA_OUTPUT_VIA_SERIAL_PORT_ENABLE;");
    expect(out).toContain("bool pulse_counter_enabled = PULSE_COUNTER_ENABLE;");
  });

  it("emits volatile system_initialized declaration", () => {
    const out = generateConfigC(makeMessagesSchema(), {});
    expect(out).toContain("volatile bool system_initialized = false;");
  });
});

describe("generateConfigC — message arrays", () => {
  it("emits all 6 message arrays in correct order", () => {
    const out = generateConfigC(makeMessagesSchema(), {});
    const positions = [
      out.indexOf("char *cw_message_templates"),
      out.indexOf("char *pip_message_templates"),
      out.indexOf("char *aprs_comment_templates"),
      out.indexOf("char *cats_comment_templates"),
      out.indexOf("char *fsq_comment_templates"),
      out.indexOf("char *ftjt_message_templates"),
    ];
    // All found
    for (const pos of positions) {
      expect(pos).toBeGreaterThanOrEqual(0);
    }
    // Strictly ascending
    for (let i = 1; i < positions.length; i++) {
      expect(positions[i]).toBeGreaterThan(positions[i - 1]);
    }
  });

  it("uses config values over schema defaults", () => {
    const config: ConfigState = {
      cw: { messages: ["$cs $altm", "$cs $loc6"] },
    };
    const out = generateConfigC(makeMessagesSchema(), config);
    expect(out).toContain('"$cs $altm",');
    expect(out).toContain('"$cs $loc6",');
    // Default value should not appear
    expect(out).not.toContain('"$cs $loc6 $altm",');
  });

  it("falls back to schema defaults when field not in config", () => {
    const out = generateConfigC(makeMessagesSchema(), {});
    expect(out).toContain('"$cs $loc6 $altm",');
  });

  it("terminates each array with NULL sentinel", () => {
    const out = generateConfigC(makeMessagesSchema(), {});
    const arrayCount = (out.match(/        NULL/g) ?? []).length;
    expect(arrayCount).toBe(6); // One per message array
  });

  it("closes each array with };", () => {
    const out = generateConfigC(makeMessagesSchema(), {});
    const closeCount = (out.match(/^};/gm) ?? []).length;
    expect(closeCount).toBe(6);
  });

  it("emits max_item_length in field doc comment", () => {
    const out = generateConfigC(makeMessagesSchema(), {});
    expect(out).toContain(" * Maximum length: 64 characters.");
    expect(out).toContain(" * Maximum length: 43 characters.");
  });

  it("escapes double-quotes in message strings", () => {
    const config: ConfigState = {
      cw: { messages: ['say "hello"'] },
    };
    const out = generateConfigC(makeMessagesSchema(), config);
    expect(out).toContain('"say \\"hello\\"",');
  });

  it("escapes backslashes in message strings", () => {
    const config: ConfigState = {
      cw: { messages: ["path\\to\\file"] },
    };
    const out = generateConfigC(makeMessagesSchema(), config);
    expect(out).toContain('"path\\\\to\\\\file",');
  });

  it("handles empty message array gracefully", () => {
    const config: ConfigState = {
      cw: { messages: [] },
    };
    const out = generateConfigC(makeMessagesSchema(), config);
    // Array still has the struct with NULL terminator
    expect(out).toContain("char *cw_message_templates[] = {");
    expect(out).toContain("        NULL");
  });
});
