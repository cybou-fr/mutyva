# Coding Agent Handoff — Work Instructions

**Product decision · 2026-10-19:** the normal experience is the running application plus **Чат с AI**. The host builds context, selects components and validates changes internally. JSON, palette controls, prompts and generations are developer tools, hidden behind Ctrl+Shift+D. Users describe a task and confirm its preview. See [14_CHAT_FIRST_UX.md](14_CHAT_FIRST_UX.md) for the primary workflow and [15_AI_CONNECTION.md](15_AI_CONNECTION.md) for implemented AI code and remaining verification.

## Mission and authoritative queue

The Foundation MVP (City Pulse → ChatGPT mutation → preview/apply → offline restart) has working code and one documented real-account Windows test. **Do not restart that implementation.** Finish its durability and clean-install gates, then deliver the Useful-app MVP: a user asks for a Task Manager, MUTYVA creates an independent local app with typed records, and a second AI request adds priority/due dates/filtering without losing tasks.

**The single current task queue is [06_IMPLEMENTATION_ROADMAP.md](06_IMPLEMENTATION_ROADMAP.md), starting at R0-02 (R0-01 passed CI on 2026-10-23).** Its R0–R4 ticket IDs, dependencies and acceptance gates supersede the old weather-first work order in this handoff. [01_MVP_SPEC.md](01_MVP_SPEC.md) remains the original **Foundation** MVP contract; do not silently replace its historical meaning. The Useful-app contract is in roadmap Slices 5–7.

Historical evidence as of 2026-10-23 (Qt 6.8): [Qt/MSVC CI](https://github.com/cybou-fr/mutyva/actions/runs/39933293033) passed 65 tests, 0 failures, and 21 individual screenshots; R0-01 and the initial typed-records/Qt palette slice have passing Windows evidence. Do not infer Android support, schema v3 completion, recovery UI or multi-app creation from these results.

## Non-negotiables

- Read `README.md`, `06_IMPLEMENTATION_ROADMAP.md`, `01_MVP_SPEC`, `02_ARCHITECTURE`, `04_DECLARATIVE_RUNTIME`, `07_TESTING_SECURITY` before changing code; consult `03_CHATGPT_AUTH` when editing auth.
- Start from root `CMakeLists.txt`; improve the starter instead of replacing Qt or introducing a web frontend.
- Use Qt 6.12.x ONLY / QML / C++20; avoid Qt WebEngine unless a proven requirement appears. OAuth opens the system browser.
- Use **only** official OpenAI Sign in with ChatGPT OAuth/Responses for AI. No paid API-key alternative in the normal flow, no cookie scraping, no other LLM.
- Don't create a server, account service, remote registry, sync, custom kernel or speculative new branch. **Small Windows CTest CI and a license are expressly required by R0-05**; neither is unrelated infrastructure.
- Prioritize user-visible end-to-end behavior, type checks and safe persistence over abstraction or styling polish.
- No arbitrary AI-generated QML/JavaScript/C++ runtime execution. Every mutation operation is typed and validated.
- Changes to app configuration must use immutable generations and preserve a previous working HEAD.
- Apply tests as code is added. Preserve the current Windows real-account AI path and offline tests. Update README/docs to match **working code**, not imagined capabilities.

## Recommended work order — follow ticket IDs

1. **Establish baseline:** compile Windows Qt/MSVC with tests, record current failures; inspect existing AppDocument, LocalDataStore, DeclarativeRuntime, AiChat and QML before changing contracts.
2. **R0-01 + R0-02:** add regression test for orphan generation gap with rollback; implement confirmed parent chain, timestamp/hash metadata, restore-by-ID, crash/corruption gates. Preserve old snapshots and HEAD safety.
3. **R0-03 + R0-04 + R0-05:** normal-user semantic diff (including defaultCity and actions), bounded/validated Open-Meteo response and deterministic fixture tests; minimum Windows CI, explicit project license/attribution review.
4. **R0-06:** clean Windows Foundation E2E, opt-in real AI test evidence separate from offline CI. Keep untouched features green.
5. **R1-01 to R1-07:** versioned v3 typed data definitions, host-owned transactional CRUD SQLite records, minimal list/form components, safe actions, schema addition, typed filters, isolated tests. First prove a manually declared Task Manager **without AI**.
6. **R2-01 to R2-04:** local catalog, per-app isolation/history/data and create/switch UX. No remote service.
7. **R3-01 to R3-05:** distinguish AI **create application** from AI **mutate existing application**; reuse validation, semantic preview, explicit Apply and crash-safe persistence. Run bounded offline fixtures and an opt-in real-account create→mutate→restart proof.
8. **R4-01 to R4-05:** only after U-MVP is accepted: Windows distribution, real Android app/runtime, protected auth lifecycle and other platform gates.

**Every ticket is a small reviewable change:** add failing test, implement, run offline suite, show user-facing behavior, document remaining risks, then mark its checkbox. Never mark a gate done only because code exists. Do not conflate Qt portability with tested ChatGPT sign-in on Android.

## Copyable demo user prompt

> Добавь в City Pulse отдельную красивую карточку влажности воздуха сразу после семидневного прогноза. Используй уже получаемое поле `current.relative_humidity_2m`. Не добавляй новые разрешения, API или исполняемый код. Сохрани существующие виджеты и все их идентификаторы.

## Rules for AI output

`AiChat` must instruct the model to emit the envelope matching `specs/designer-response.schema.json`, containing a mutation matching `specs/mutation.schema.json`, not markdown. The current implementation shows a useful error on invalid output and preserves HEAD; retries are manual. A future bounded repair request must use only a validation summary and must not send private data back.

## Review gates

- Actual working compile and smoke, not merely source written.
- Network and OAuth error paths proven.
- UI change is clearly visible and persists across process restart.
- Import/export never includes protected credentials.
- Demo app continues to run with AI disabled.
- For R1–R3, existing local record IDs/values survive schema/UI mutations and offline restart.
- Actions and model migrations are visible in user-readable preview; user data is not sent to AI by default.
- Each R0–R4 acceptance gate is recorded with tests/evidence before its checkbox changes.

## Definition of the “done” answer to the team

Report: files changed; build/test commands run; evidence/screenshot; passed/failed checklist; remaining blockers explicitly. No claims about iOS/Android or auth unless actually tested.
