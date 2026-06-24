#!/usr/bin/env bun
/**
 * RS41ng Config Generator
 *
 * Reads config.yaml + web/config-schema.yaml, validates, and generates
 * src/config_generated.h and src/config_generated.c (or custom paths) for the firmware build.
 *
 * Usage:
 *   bun run scripts/generate_config.ts <config.yaml>
 *   bun run scripts/generate_config.ts <config.yaml> <config_generated.h> <config_generated.c>
 *
 * Exit codes: 0 = success, 1 = usage/parse error, 2 = validation error
 *
 * Generator logic is shared with the web configurator via web/src/config/.
 */

import { readFileSync, writeFileSync } from "fs";
import { resolve, dirname } from "path";
import { load as yamlLoad } from "js-yaml";

// Shared schema/validation/generator logic — the single source of truth, reused
// here so the CLI build and the web configurator never diverge.
import type { Schema } from "../web/src/types/schema";
import type { ConfigState, ConfigSection } from "../web/src/types/config";
import { buildDefaults } from "../web/src/config/defaults";
import { validateConfig } from "../web/src/config/validation";
import { generateConfigH } from "../web/src/config/generate-header";
import { generateConfigC } from "../web/src/config/generate-source";

// ─── Helpers ──────────────────────────────────────────────────────────────────

function die(message: string, code: number = 1): never {
  console.error(`ERROR: ${message}`);
  process.exit(code);
  throw new Error(message); // unreachable; satisfies TypeScript never return
}

/**
 * Merge the user's (sparse) config.yaml over the schema defaults to produce a
 * fully-populated state, which is what validateConfig() expects (it mirrors the
 * always-populated store the web UI validates).
 */
function mergeWithDefaults(schema: Schema, userConfig: unknown): ConfigState {
  const merged = buildDefaults(schema);
  if (userConfig && typeof userConfig === "object") {
    for (const [sectionKey, section] of Object.entries(userConfig)) {
      if (section && typeof section === "object" && !Array.isArray(section)) {
        merged[sectionKey] = {
          ...(merged[sectionKey] ?? {}),
          ...(section as ConfigSection),
        };
      }
    }
  }
  return merged;
}

// ─── Main ─────────────────────────────────────────────────────────────────────

function main() {
  const args = process.argv.slice(2);
  if (args.length < 1 || args.length === 2 || args.length > 3) {
    console.error(
      "Usage: bun run scripts/generate_config.ts <config.yaml> [config_generated.h] [config_generated.c]\n" +
        "       Default output: src/config_generated.h and src/config_generated.c"
    );
    process.exit(1);
  }

  const scriptDir = dirname(new URL(import.meta.url).pathname);
  const repoRoot = resolve(scriptDir, "..");

  const configYamlPath = args[0];
  const configHPath =
    args[1] ?? resolve(repoRoot, "src", "config_generated.h");
  const configCPath =
    args[2] ?? resolve(repoRoot, "src", "config_generated.c");

  // Find schema relative to this script (scripts/ -> project root -> web/)
  const schemaPath = resolve(scriptDir, "..", "web", "config-schema.yaml");

  // Load schema
  let schema: Schema;
  try {
    schema = yamlLoad(readFileSync(schemaPath, "utf8")) as Schema;
  } catch (e: any) {
    die(`Failed to load schema from ${schemaPath}: ${e.message}`);
  }

  // Load user config
  let userConfig: ConfigState;
  try {
    userConfig = (yamlLoad(readFileSync(configYamlPath, "utf8")) as ConfigState) ?? {};
  } catch (e: any) {
    die(`Failed to load config from ${configYamlPath}: ${e.message}`);
  }

  // Validate via the shared validator (same rules and conflict detection as the web UI)
  const result = validateConfig(schema, mergeWithDefaults(schema, userConfig));
  if (!result.valid) {
    for (const [path, fieldErrors] of Object.entries(result.fieldErrors)) {
      for (const err of fieldErrors) {
        console.error(`Validation error: ${path}: ${err}`);
      }
    }
    for (const conflict of result.conflicts) {
      console.error(`Validation error: ${conflict}`);
    }
    process.exit(2);
  }

  // Generate using the shared generator modules
  const configH = generateConfigH(schema, userConfig);
  const configC = generateConfigC(schema, userConfig);

  // Write
  writeFileSync(configHPath, configH, "utf8");
  writeFileSync(configCPath, configC, "utf8");

  console.log(`Generated ${configHPath}`);
  console.log(`Generated ${configCPath}`);
}

main();
