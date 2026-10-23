# Architecture Decision Record — Current Decisions Only

**Product decision · 2026-10-19:** the normal experience is the running application plus **Чат с AI**. The host builds context, selects components and validates changes internally. JSON, palette controls, prompts and generations are developer tools, hidden behind Ctrl+Shift+D. Users describe a task and confirm its preview. See [14_CHAT_FIRST_UX.md](14_CHAT_FIRST_UX.md) for the primary workflow and [15_AI_CONNECTION.md](15_AI_CONNECTION.md) for implemented AI code and remaining verification.

This is a **current-state** record, not a changelog of rejected speculative versions.

| Topic | Decision |
|---|---|
| Product | MUTYVA, one cross-platform **self-evolving application**, not a full operating system |
| Main language | C++20 |
| GUI | **Qt 6.12.x only** / QML / Qt Quick Controls; no Qt 6.8 fallback |
| Platforms | Windows, Linux, macOS, Android, iOS target |
| Application format | Strict JSON declarations of component compositions |
| AI | Sign in with ChatGPT (eligible Plus/Pro plan usage); only AI provider in scope |
| AI usage | Creating/changing app definitions; **not** needed for normal saved-app execution |
| Data | Current: trusted Open-Meteo public HTTPS + local SQLite. Target: typed snapshots from vetted adapters, never AI-issued arbitrary URLs |
| Persistence | Local immutable generations; no mandatory cloud services |
| Component library | Current 21 trusted kinds + machine-readable recipes; future versioned typed ports/events/layout, no model-generated QML/JS/SQL |
| User publishing/sync | **Out of scope**; no server or remote registry |
| Sandbox | No arbitrary executable code in model-produced mutations or imported components |
| Customization | Conversational AI composer in MVP, visual RAD designer later |
| Development | Small vertical slices; code/docs agree; prefer direct updates to main |

## Why not generate QML source per prompt?

The runtime must work on mobile, not require compilers on-device, preserve existing state and handle untrusted AI output. Declarative trees allow fast validation, incremental update and reproducibility. Native code extensions can be separately designed after the product is already useful.

## Why no initial DB explorer?

Connecting arbitrary PostgreSQL/REST sources is a flagship direction, but it increases permissions, secrets, schema inference and CRUD risk. Live public Open-Meteo is a faster and safer proof of the *same UI+data+AI mutation machinery*. Do DB connectors after the working end-to-end demo, not before.

## 2026-10-23: dataflow, UI composition and connectors

- **Single source of runtime truth:** `specs/app.schema.json`, `runtime/component-catalog.json`, trusted host implementations. The new `runtime/designer-capabilities.json` is compact AI guidance checked against them, not a new executable plugin API.
- **Typed graph before new widgets:** no general-purpose computation or workflow language; each future binding uses a declared read-only selector and compatible component port; each event dispatches one host-validated action.
- **Layout is declarative, Qt-native and responsive:** stable hierarchy with bounded tokens, not freehand QML, CSS, absolute coordinates or AI-authored scripts. Default accessible on Windows and Android-sized surfaces.
- **External adapters are vetted and read-only first:** Open-Meteo is present, general arbitrary REST/SQL is absent. Future sources require allowlisted domains, explicit permissions, schema normalization, stale/error states and independent offline tests.
- **Qt 6.12 CI is still unverified** until the GitHub Secrets gate for the official installer is fulfilled; historical Qt 6.8 success is not equivalent.
- **Current v2 vs proposed v3 must never be conflated** in prompts, examples, tickets or release notes. See [20](20_COMPOSITION_CONTRACT.md), [21](21_CONNECTOR_CONTRACT.md), [22](22_AI_DESIGN_PLAYBOOK.md).
