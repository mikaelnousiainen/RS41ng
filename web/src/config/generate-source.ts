import type { Schema } from "../types/schema";
import type { ConfigState } from "../types/config";
import { getField } from "./schema-registry";
import { escapeString } from "./format-value";

const CONFIG_C_TEMPLATE_VAR_COMMENT = `/**
 * The tracker firmware will transmit each of the message templates defined here in rotation, one by one,
 * starting again from the beginning once the last message for a particular mode is transmitted.
 *
 * Supported variable references in templates:
 *
 * $cs - Call sign
 * $loc4 - Locator (4 chars)
 * $loc6 - Locator (6 chars)
 * $loc8 - Locator (8 chars)
 * $loc12 - Locator (12 chars)
 * $bv - Battery voltage in millivolts (up to 4 chars)
 * $te - External temperature in C (up to 3 chars)
 * $ti - Internal temperature in C (up to 3 chars)
 * $hu - Humidity percentage (up to 3 chars)
 * $pr - Atmospheric pressure in millibars (up to 4 chars)
 * $tow - GPS time of week in milliseconds
 * $hh - Current hour (2 chars)
 * $mm - Current minute (2 chars)
 * $ss - Current second (2 chars)
 * $sv - GPS satellites visible (up to 2 chars)
 * $lat - Latitude in degrees * 1000 (up to 6 chars)
 * $lon - Longitude in degrees * 1000 (up to 6 chars)
 * $alt - Altitude in meters (up to 5 chars)
 * $gs - Ground speed in km/h (up to 3 chars)
 * $cl - Climb in m/s (up to 2 chars)
 * $he - Heading in degrees (up to 3 chars)
 * $pc - Pulse counter value (wraps to zero at 65535, 16-bit unsigned value)
 * $ri - Radiation intensity in µR/h (up to 5 chars)
 * $dc - Data counter value, increases by one every time telemetry is read (wraps to zero at 65535, 16-bit unsigned value)
 * $gu - GPS data update indicator, 1 if GPS data was updated since time telemetry was read, 0 otherwise
 * $ct - Clock calibration trim value (0-31, only for DFM-17)
 * $cc - Clock calibration change count (only for DFM-17)
 */`;

/** Fixed field order matching original config.c message array declaration order */
const MESSAGE_FIELD_ORDER = [
  { section: "cw", field: "messages" },
  { section: "pip", field: "messages" },
  { section: "aprs", field: "messages" },
  { section: "cats", field: "messages" },
  { section: "fsq", field: "messages" },
  { section: "ft8", field: "messages" },
] as const;

/**
 * Generate the content of src/config.c from the schema and current config state.
 * Output is identical to what scripts/generate_config.ts produces for the same input.
 */
export function generateConfigC(schema: Schema, config: ConfigState): string {
  const lines: string[] = [];

  lines.push(CONFIG_C_TEMPLATE_VAR_COMMENT, "");
  lines.push('#include <stdlib.h>', '#include "config.h"', "");

  // Build-time notice that the generated configuration is in effect. Emitted here
  // (a single-compile translation unit) rather than in config.h, which is included
  // by dozens of files and would print the message once per file.
  lines.push(
    '#pragma message("RS41ng: building with GENERATED configuration (config_generated.h, from config.yaml)")',
    ""
  );

  lines.push(
    "bool leds_enabled = LEDS_ENABLE;",
    "bool bmp280_enabled = SENSOR_BMP280_ENABLE;",
    "bool radsens_enabled = SENSOR_RADSENS_ENABLE;",
    "bool si5351_enabled = RADIO_SI5351_ENABLE;",
    "bool gps_nmea_output_enabled = GPS_NMEA_OUTPUT_VIA_SERIAL_PORT_ENABLE;",
    "bool pulse_counter_enabled = PULSE_COUNTER_ENABLE;",
    "",
    "volatile bool system_initialized = false;",
    ""
  );

  // Message template arrays in fixed declaration order
  for (const entry of MESSAGE_FIELD_ORDER) {
    const field = getField(schema, entry.section, entry.field);
    if (!field) continue;

    const templates: string[] =
      (config[entry.section]?.[entry.field] as string[] | undefined) ??
      (field.default as string[] | undefined) ??
      [];

    lines.push("/**", ` * ${field.label}`);
    if (field.validation?.max_item_length) {
      lines.push(
        ` * Maximum length: ${field.validation.max_item_length} characters.`
      );
    }
    lines.push(" */");
    lines.push(`char *${field.define}[] = {`);
    for (const item of templates) {
      lines.push(`        "${escapeString(String(item))}",`);
    }
    lines.push("        NULL", "};", "");
  }

  return lines.join("\n");
}
