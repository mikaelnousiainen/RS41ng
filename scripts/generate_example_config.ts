#!/usr/bin/env bun
/**
 * Regenerates config.yaml.example from web/config-schema.yaml.
 *
 * The example file is a fully-documented reference containing every section and
 * setting at its default value. Run this whenever the schema changes:
 *
 *   bun run scripts/generate_example_config.ts
 *
 * The test web/src/test/config/example-config.test.ts fails if the committed file
 * drifts from the generator output, so CI catches a forgotten regeneration.
 */

import { readFileSync, writeFileSync } from "fs";
import { resolve, dirname } from "path";
import { load as yamlLoad } from "js-yaml";

import type { Schema } from "../web/src/types/schema";
import { generateExampleYaml } from "../web/src/config/generate-example";

const scriptDir = dirname(new URL(import.meta.url).pathname);
const repoRoot = resolve(scriptDir, "..");

const schema = yamlLoad(
  readFileSync(resolve(repoRoot, "web", "config-schema.yaml"), "utf8")
) as Schema;

const outPath = resolve(repoRoot, "config.yaml.example");
writeFileSync(outPath, generateExampleYaml(schema), "utf8");
console.log(`Generated ${outPath}`);
