# MUTYVA — MVP Delivery Pack

**Status:** implementation specification + Qt demo starter, 2026-10-18. **Language:** Russian.  
**Product:** One AI-driven Qt application that can become many applications; after a mutation is saved, it executes independently of AI.

## Product in one sentence

MUTYVA is a local, declarative, cross-platform application runtime with a component palette, immutable mutation history and an AI designer authorized via **Sign in with ChatGPT**.

**Русская краткая инструкция:** [START_HERE_RU.md](START_HERE_RU.md). Подробные инженерные спецификации ниже даны на английском для удобства программирующих агентов.

## What this package contains

| Path | Why open it |
|---|---|
| `docs/00_PRODUCT_VISION.md` | Unambiguous product vision and what NOT to build |
| `docs/01_MVP_SPEC.md` | User journeys, screens and definition of done |
| `docs/02_ARCHITECTURE.md` | Qt/C++ modules, boundaries, storage, event flow |
| `docs/03_CHATGPT_AUTH.md` | Verified Sign in with ChatGPT integration blueprint |
| `docs/04_DECLARATIVE_RUNTIME.md` | Declarative schema, controlled mutations and history |
| `docs/05_DEMO_CITY_PULSE.md` | Public APIs, response mapping and demo narrative |
| `docs/06_IMPLEMENTATION_ROADMAP.md` | Thin vertical slices, tasks and milestones |
| `docs/07_TESTING_SECURITY.md` | End-to-end acceptance, threats, offline and error tests |
| `docs/08_PLATFORMS_RELEASE.md` | Windows, Android, macOS, Linux and iOS strategy |
| `docs/09_ENGINEERING_DECISIONS.md` | Binding decisions and non-goals |
| `docs/10_AGENT_HANDOFF.md` | Copyable implementation instructions for coding agents |
| `specs/` | JSON schemas (application and mutation) |
| `examples/` | Declarative City Pulse and an example mutation |
| `starter/` + root `CMakeLists.txt` | Small Qt 6 QML/C++ starter with live public APIs and local versioned UI |
| `tools/validate_examples.py` | Standard-library example checks |

## Quickest path to a visible result

1. Install **Qt 6.8+** with Qt Quick, Qt Quick Controls and the desktop kit, CMake and C++20 toolchain.
2. Open the root `CMakeLists.txt` in Qt Creator, or use `qt-cmake -S . -B build` then `cmake --build build --config Release`.
3. Run `mutyva-demo` with internet access. It searches **Paris** and displays weather, a seven-day forecast and air quality via Open-Meteo.
4. Press **+ Виджет влажности**: the JSON-driven widget list mutates and a new immutable local generation is saved.
5. Restart: the interface remains changed, without an AI request. Press **Откат поколения** or **Сбросить интерфейс**.
6. Implement `docs/03_CHATGPT_AUTH.md` and connect the real AI mutation request to the schema validation and commit path.

**Important:** the starter is a *bootstrap demonstration*, not the finished MVP: its mutation is deterministic, and it deliberately does **not** include actual ChatGPT OAuth, AI calls, SQL, production-quality security validation or iOS verification. All are described as explicit work items rather than being misrepresented as already built.

## MVP demo sequence

**Real public APIs → user asks for a change → ChatGPT returns declarative mutation → validate and preview → apply atomically → save generation → restart and work without ChatGPT.**

## No unnecessary infrastructure

No MUTYVA server, package registry, accounts, marketplace, backend, synced state, Ollama, alternate models, Flutter, Avalonia, custom kernel or Assembly. A user's optional **ChatGPT authorization** is not a MUTYVA account.

## Key technical choices

- **Qt 6.8+**, **C++20**, **QML/Qt Quick**, **CMake**.
- Local **SQLite** for catalog, history, settings and cached data in full MVP; starter uses local JSON snapshots to reduce bootstrap complexity.
- Declarative and schema-validated application trees; QML renders **prebuilt registered components**. Do not load untrusted generated QML as code.
- OpenAI **Sign in with ChatGPT** for eligible open-source local applications, OAuth2 Authorization Code + PKCE + OIDC; no unofficial ChatGPT web endpoints or stolen sessions.
- Open-Meteo for a noncommercial demo; licensing/attribution requirements apply.

## Authoritative links (review again before implementation)

- Sign in with ChatGPT overview: https://developers.openai.com/siwc/token-sharing-open-source
- Sign-in and registration: https://developers.openai.com/siwc/token-sharing-open-source/sign-in
- Models and inference: https://developers.openai.com/siwc/token-sharing-open-source/models-and-inference
- Preview limitations: https://developers.openai.com/siwc/token-sharing-open-source/preview-limitations
- Qt Network Authorization: https://doc.qt.io/qt-6/qoauth2authorizationcodeflow.html
- Qt supported platforms: https://doc.qt.io/qt-6/supported-platforms.html
- Open-Meteo: https://open-meteo.com/en/docs and https://open-meteo.com/en/terms

## Documentation discipline

These documents describe **the proposed MVP target**, not released capabilities. Update docs as code evolves; avoid a changelog of speculative versions. Do not proclaim a feature done until the acceptance tests in `docs/07_TESTING_SECURITY.md` pass.
