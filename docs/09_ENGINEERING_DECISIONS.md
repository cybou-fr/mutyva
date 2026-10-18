# Architecture Decision Record — Current Decisions Only

**Product decision · 2026-10-19:** the normal experience is the running application plus **Чат с AI**. The host builds context, selects components and validates changes internally. JSON, palette controls, prompts and generations are developer tools, hidden behind Ctrl+Shift+D. Users describe a task and confirm its preview. See [14_CHAT_FIRST_UX.md](14_CHAT_FIRST_UX.md) for the primary workflow and [15_AI_CONNECTION.md](15_AI_CONNECTION.md) for implemented AI code and remaining verification.

This is a **current-state** record, not a changelog of rejected speculative versions.

| Topic | Decision |
|---|---|
| Product | MUTYVA, one cross-platform **self-evolving application**, not a full operating system |
| Main language | C++20 |
| GUI | Qt 6 / QML / Qt Quick Controls |
| Platforms | Windows, Linux, macOS, Android, iOS target |
| Application format | Strict JSON declarations of component compositions |
| AI | Sign in with ChatGPT (eligible Plus/Pro plan usage); only AI provider in scope |
| AI usage | Creating/changing app definitions; **not** needed for normal saved-app execution |
| Data | First live app uses noncommercial Open-Meteo public HTTPS; local catalog/history |
| Persistence | Local immutable generations; no mandatory cloud services |
| Component library | Built-in official components + locally created safe compositions |
| User publishing/sync | **Out of scope**; no server or remote registry |
| Sandbox | No arbitrary executable code in model-produced mutations or imported components |
| Customization | Conversational AI composer in MVP, visual RAD designer later |
| Development | Small vertical slices; code/docs agree; prefer direct updates to main |

## Why not generate QML source per prompt?

The runtime must work on mobile, not require compilers on-device, preserve existing state and handle untrusted AI output. Declarative trees allow fast validation, incremental update and reproducibility. Native code extensions can be separately designed after the product is already useful.

## Why no initial DB explorer?

Connecting arbitrary PostgreSQL/REST sources is a flagship direction, but it increases permissions, secrets, schema inference and CRUD risk. Live public Open-Meteo is a faster and safer proof of the *same UI+data+AI mutation machinery*. Do DB connectors after the working end-to-end demo, not before.
