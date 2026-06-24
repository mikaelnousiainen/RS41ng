import type { Schema } from "@/types/schema";

// Imported at build time by vite-plugin-yaml — schema is baked into the bundle,
// no runtime network request needed.
import rawSchema from "../../config-schema.yaml";

export const schema: Schema = rawSchema as Schema;
