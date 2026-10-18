# MUTYVA — Product Vision

## Promise

**One App. Infinite Forms.** An AI-driven, local-first Qt application that becomes the tool a person needs. The application itself changes shape; it does not continuously generate separately installed EXE/APK packages.

## Product principles (decisions)

1. **One runtime, many compositions.** Every saved application is a declaration of screens, components, data sources, bindings and actions executed by MUTYVA.
2. **AI designs; MUTYVA runs.** ChatGPT helps create/modify apps, but no model is needed for everyday interaction with saved apps.
3. **No AI lock on data.** GUI actions, API retrieval, local editing, search, navigation, calculations and previously configured rules run without ChatGPT.
4. **Declarative by default.** AI proposes *typed changes*; the trusted host applies them; arbitrary QML/JS/native executables are not accepted as community components.
5. **Visible evolution.** A successful mutation creates an immutable generation. The user can preview, compare and restore an earlier app configuration; external side effects are not undone.
6. **AI is optional at runtime, built into design mode.** Without sign-in the demo and existing apps continue to run; changes via natural language require supported ChatGPT authorization and connectivity.
7. **Local first.** Everything except explicitly requested ChatGPT inference and selected public API calls resides on the user's device.
8. **Future cross-platform.** Windows, Linux, macOS, Android, iOS share one document format and Qt codebase. UI layout changes with screen size/input mode.
9. **No platform/provider theater.** Do not pretend a Qt runtime is a new OS. Do not embed an unverified ChatGPT integration or call unofficial web APIs.
10. **Time-to-value.** Working data first, then mutation engine, then advanced visual designer.

## MVP end-to-end user story

Launch MUTYVA → see City Pulse with real Paris weather/air readings → optionally choose a different city → sign in with ChatGPT → ask “Добавь отдельную карточку влажности после прогноза” → inspect preview → Apply → a new widget appears and generation increments → restart offline → previously saved UI renders with cached data and no AI.

## What makes it distinct

It is not another prompt-to-source-code tool. It is a persistent **application-runtime mutation system** with a stable palette and data/action bindings. Successful compositions become real, offline-runnable user applications. The owner can also change them manually through a later RAD editor.

## MVP must NOT include

- Own operating system, kernel, bootloader, AOSP builds.
- Server, component marketplace, Forgejo/GitHub component fetching, user registration, sync.
- Other AI providers, local Ollama, model-router logic.
- A complete low-code/RAD IDE, arbitrary scripting or native plugin loading.
- Commercial payments, remote deployment, multi-user roles and complex DB integrations.
- A custom programming language, generic reactive query optimizer or full Nix clone.

## Product-language precision

“App” is a saved declarative composition, **not** a separate compiled program. “Component” is a registered, trusted Qt implementation or a safe composition. “Mutation” is a validated change transaction. “Generation” is an immutable version snapshot. “History rollback” changes configuration only; it must never imply undoing external real-world effects.
