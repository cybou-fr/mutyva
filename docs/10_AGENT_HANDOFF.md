# Coding Agent Handoff — Work Instructions

## Mission

Implement the smallest production-shaped, end-to-end MUTYVA AI-driven mutation loop in the existing Qt starter. The desired experience: run City Pulse, sign in with ChatGPT using official OSS flow, ask in Russian for a humidity card, preview, accept, persist and reopen the altered app **without AI**.

## Non-negotiables

- Read `README.md`, `01_MVP_SPEC`, `02_ARCHITECTURE`, `03_CHATGPT_AUTH`, `04_DECLARATIVE_RUNTIME`, `07_TESTING_SECURITY` before changing code.
- Start from root `CMakeLists.txt`; improve the starter instead of replacing Qt or introducing a web frontend.
- Use Qt 6.8+/QML/C++20; avoid Qt WebEngine unless a proven requirement appears. OAuth opens the system browser.
- Use **only** official OpenAI Sign in with ChatGPT OAuth/Responses for AI. No paid API-key alternative in the normal flow, no cookie scraping, no other LLM.
- Don't create a server, account service, registry, sync, unrelated CI, custom kernel or new branch.
- Prioritize user-visible end-to-end behavior, type checks and safe persistence over abstraction or styling polish.
- No arbitrary AI-generated QML/JavaScript/C++ runtime execution. Every mutation operation is typed and validated.
- Changes to app configuration must use immutable generations and preserve a previous working HEAD.
- Apply tests as code is added. Update README/docs to match **working code**, not imagined capabilities.

## Recommended work order

1. **Build and inspect** current starter on Windows. Fix immediate QML/CMake issues. Confirm City Pulse uses public APIs.
2. **Core document model**: add validation against `specs/app.schema.json`, known kinds, stable IDs and type-safe bindings.
3. **Mutation engine**: load `examples/mutations/add-humidity-card.json`, preview, apply only on explicit confirmation; invalid mutation leaves no traces.
4. **Generations**: add list/restore UI, atomic writes and tests; keep prior generations immutable.
5. **ChatGPT OAuth**: follow current official SIWC docs literally; secure session management, model discovery, streaming inference; prove a harmless hello request.
6. **AI mutation**: send current app, allowed catalog and user intent; parse JSON and apply through exactly the same mutation engine as the local fixture.
7. **Clean installation E2E**: no sign-in normal mode, then account auth + mutation, then offline restart.
8. **Android smoke**: responsive/touch, actual device OAuth callback test.

## Copyable demo user prompt

> Добавь в City Pulse отдельную красивую карточку влажности воздуха сразу после семидневного прогноза. Используй уже получаемое поле `current.relative_humidity_2m`. Не добавляй новые разрешения, API или исполняемый код. Сохрани существующие виджеты и все их идентификаторы.

## Rules for AI output

`AiDesigner` must instruct the model to emit a JSON object matching `specs/mutation.schema.json`, not markdown. Re-prompt once with a *validation summary* if invalid; do not send private data back. Any continued failure shows a useful error and preserves HEAD.

## Review gates

- Actual working compile and smoke, not merely source written.
- Network and OAuth error paths proven.
- UI change is clearly visible and persists across process restart.
- Import/export never includes protected credentials.
- Demo app continues to run with AI disabled.

## Definition of the “done” answer to the team

Report: files changed; build/test commands run; evidence/screenshot; passed/failed checklist; remaining blockers explicitly. No claims about iOS/Android or auth unless actually tested.
