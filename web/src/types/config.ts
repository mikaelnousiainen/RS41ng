/** A single field value in the user's firmware config */
export type ConfigValue = string | number | boolean | string[];

/** One section of config values (e.g. all fields under "aprs:") */
export type ConfigSection = Record<string, ConfigValue>;

/** Full firmware config state — mirrors the config.yaml structure */
export type ConfigState = Record<string, ConfigSection>;
