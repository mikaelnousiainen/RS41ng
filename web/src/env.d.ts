/// <reference types="vite/client" />

declare module "*.vue" {
  import type { DefineComponent } from "vue";
  const component: DefineComponent<object, object, unknown>;
  export default component;
}

declare module "*.yaml" {
  const content: unknown;
  export default content;
}

declare module "*.yml" {
  const content: unknown;
  export default content;
}
