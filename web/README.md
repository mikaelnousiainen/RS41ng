# RS41ng Web Configurator

A browser-based tool that guides users through configuring, building, and flashing RS41ng firmware. It is a
static single-page app (Vue 3 + TypeScript + Vite + Tailwind + Pinia) hosted on GitHub Pages. There is no
backend: configuration is generated in the browser, the firmware is compiled locally via Docker, and flashing
is done over WebUSB.

This document is for developers working on the configurator. End-user instructions live in the
[top-level README](../README.md#web-configurator-easiest-way-to-get-started).

## Local development

The canonical package manager is **[Bun](https://bun.sh/)** (the Docker build and the config generator use it
too). From this `web/` directory:

```bash
bun install              # install dependencies (uses bun.lock)
bun run dev              # start the Vite dev server
bun run test             # run the Vitest suite
bun run build            # type-check (vue-tsc) + production build into web/dist
bun run preview          # serve the production build locally
```

`bun run dev:https` serves over HTTPS, which some browsers require to expose WebUSB on non-localhost origins.

## Architecture

The configurator is **schema-driven**: a single YAML file describes every firmware option, and the form,
validation, and config generators are all derived from it.

```
config-schema.yaml ──(vite-plugin-yaml)──> schema-loader.ts
                                                  │
        ┌─────────────────────────────────────────┼───────────────────────────────┐
        ▼                     ▼                     ▼               ▼                ▼
  schema-registry.ts     defaults.ts         validation.ts   visibility.ts   generate-*.ts
   (indexed access)   (default values)    (+ conflict rules)  (visible_when)  (yaml/h/c output)
        │                     │                     │               │                │
        └─────────────────────┴──────────┬──────────┴───────────────┘                │
                                          ▼                                           │
                            stores/ (config, ui, flasher)                             │
                                          ▼                                           │
                           components/ (wizard steps + fields) ◄──────────────────────┘
```

Key directories under `src/`:

- **`config/`** — framework-agnostic logic: schema access, defaults, validation, visibility, and the
  `generate-yaml` / `generate-header` / `generate-source` generators. These have no Vue dependency and are
  unit-tested in isolation.
- **`config/strategies/` + `lib/webstlink/`** — the flashing layer. `FlashStrategy` is a pluggable interface
  (`flash-strategy.ts`); `webstlink-strategy.ts` is the WebUSB/ST-LINK implementation built on a vendored,
  locally-patched copy of [devanlai/webstlink](https://github.com/devanlai/webstlink) (including a new STM32L4
  flash driver). See the strategy and `lib/webstlink/webstlink.d.ts` for the API surface.
- **`stores/`** — Pinia stores for config state, wizard/UI state, and the flasher state machine.
- **`components/`** — the wizard (`steps/`), the schema-driven form (`config/` + `config/fields/`), the build
  guide (`build/`), and the flashing UI (`flash/`). Field inputs share `FieldShell.vue` + the `useField`
  composable.

### Shared code with the CLI generator

`scripts/generate_config.ts` (in the repo root, run under Bun inside the Docker build) imports the same
`config/` modules — `buildDefaults`, `validateConfig`, `generateConfigH`, `generateConfigC`. The browser and
the CLI therefore produce identical output from the same `config.yaml`. `src/test/config/example-config.test.ts`
runs the repo-root `config.yaml.example` through that shared pipeline as a regression guard.

The repo-root **`config.yaml.example`** is itself generated from the schema by
`config/generate-example.ts` (every section and field, at its default value, with documentation comments).
Regenerate it after any schema change:

```bash
bun run scripts/generate_example_config.ts   # from the repo root
```

`example-config.test.ts` fails if the committed file drifts from the generator or is missing any schema
field, so a forgotten regeneration is caught in CI.

### Flash protection: factory unlock vs. lock (known limitation)

Factory RS41/DFM-17 sondes ship with flash **read protection (RDP)** enabled. The configurator
auto-detects this on connect and offers **Remove protection** — this is the supported, verified path
(F1 and L4) that a first-time user needs.

The reverse operation — the **"Lock MCU" developer tool** (re-enabling RDP) — is **experimental** and
surfaced as such in the UI (collapsed "Developer tools" section with an *Experimental* badge). It is **not
needed for normal use**, and on some STM32F1 clones the RDP option-byte write may not take effect; STM32F1
also requires a power cycle for the change to apply. L4 lock/unlock round-tripping is not fully verified.
If the in-browser unlock ever fails on a particular sonde, the OpenOCD command-line unlock documented in the
[top-level README](../README.md) is the reliable fallback.

## Adding or changing a firmware option

The schema is the single source of truth — in most cases you only edit `config-schema.yaml`:

1. Add or modify the field under the appropriate section (set `define`, `type`, `default`, `tier`, `label`,
   `description`, and optional `validation` / `visible_when`).
2. The form, defaults, validation, and all generators pick it up automatically — no component changes needed
   for standard field types.
3. Run `bun run test` and `bun run build`. Add or update tests under `src/test/` for non-trivial logic.

New field *types* (beyond string/bool/integer/hex/frequency/char/enum/enum_constant/string_array) require a
new field component plus a mapping entry in `components/config/FieldRenderer.vue`.

## Deployment (GitHub Pages)

Deployment is automated by [`.github/workflows/deploy-web.yml`](../.github/workflows/deploy-web.yml): on every
push to `main` that touches `web/**`, it installs with Bun, runs the tests, builds, and publishes `web/dist`
to GitHub Pages.

One-time repository setup (cannot be done by the workflow): **Settings → Pages → Build and deployment →
Source = "GitHub Actions"**. The site is then served at `https://<owner>.github.io/<repo>/`. `vite.config.ts`
sets `base: "./"` (relative asset paths) so the app works from that project subpath without further
configuration.

The Bun version in the workflow is pinned and should be kept in sync with the `Dockerfile`.
