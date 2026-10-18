# Implementation Roadmap — fastest path to MVP

**Product decision · 2026-10-19:** the normal experience is the running application plus **Чат с AI**. The host builds context, selects components and validates changes internally. JSON, palette controls, prompts and generations are developer tools, hidden behind Ctrl+Shift+D. Users describe a task and confirm its preview. See [14_CHAT_FIRST_UX.md](14_CHAT_FIRST_UX.md) for the primary workflow and [15_AI_CONNECTION.md](15_AI_CONNECTION.md) for implemented AI code and remaining verification.

**Workflow:** straight to `main`, small commits, docs kept in sync; avoid speculative release-number archaeology. Acceptance is measured by demonstrable behavior, not number of classes added.

## Slice 0 — bootstrap and live API (start immediately)

- [x] Windows MSVC / Qt 6.12 project compiles with CMake using C++20 and QML (other kits unverified).
- [ ] City Search -> Open-Meteo geocoding -> forecast and air-quality HTTPS.
- [ ] Show loading, error, empty state, attribution and timestamps.
- [ ] UI usable from ~360 dp width to desktop width.
- [ ] Add a lightweight smoke test and save a screenshot for baseline.

**Delivered partially by `starter/`;** missing polish, user selection among homonymous cities and offline fixture mode are still work.

## Slice 1 — declarative UI contract

- [x] Validate against bundled `app.schema.json` before rendering, saving and restoring (checker supports this schema's vocabulary).
- [x] Catalog of twelve kinds: five weather widgets and seven general components, per-kind properties, fixed weather bindings and typed local-data actions.
- [x] `WidgetHost` renders known kinds; titles are read from the declaration.
- [x] Separate weather service with typed read-only Q_PROPERTY data; fixed binding contract per kind, no expression evaluation.
- [x] Support `addWidget`, `removeWidget`, `updateProperty`, `moveWidget` with reference checks and ordered batch semantics.
- [x] Tests for malformed docs, unknown kinds, duplicate widget IDs, forbidden actions; offline QML preview/render tests.

**Acceptance path:** edit a declaration in the built-in JSON editor (or paste a hand-edited `.mutyva.json`), validate/preview and Apply without recompiling. File-picker import/export remains future work.

## Slice 2 — immutable mutation history

- [ ] Persist application generations and HEAD locally using durable atomic writes.
- [ ] Add description, timestamp, parent generation and SHA-256 of canonical JSON.
- [x] Preview a candidate mutation and choose Apply/Cancel; cancellation does not write to disk.
- [ ] History UI: list generations and restore any version as a **new** generation.
- [ ] On crash/disk-full, previous generation remains intact.

**Acceptance:** apply >3 changes, restart; restore any version; never lose the baseline. Starter has a limited file-based precedent.

Current persistence adds a writer lock, stale-HEAD rejection and avoids reusing orphan snapshot filenames. Corrupt saved documents are rejected without overwriting history. Full recovery, metadata/hash/parent tracking and the selectable history browser remain unfinished.

## Slice 3 — Sign in with ChatGPT (critical feasibility gate)

Preparation for AI integration is now implemented locally: shared component catalog with property/binding/action metadata, trusted designer instructions, bounded inspectable context and a checked proposal/clarification/unsupported response envelope. See `11_AI_DESIGN_CONTRACT.md`. Windows ChatSession and AiChat now implement the first browser sign-in and Responses path; real-account verification, persistent sessions and refresh remain pending (doc15).

General palette v2 is implemented with parent containers, append-only v1 migration, an interactive Notes composition and separate persistent SQLite input/list data with transactional actions and stale-writer protection. See `12_GENERAL_PALETTE.md` and `13_LOCAL_DATA.md`. Generic business records and DataTable remain future work.

- [x] Implement first Windows dynamic OAuth registration with host ID, PKCE, state and nonce (live verification pending).
- [x] Implement Windows RS256/CNG verification of ID token and granted plan scope, with offline negative tests (live verification pending).
- [ ] Secure OS credential storage; refresh and disconnect.
- [x] Implement `GET /v1/models` under OAuth account and automatic visible-model selection (live verification pending).
- [x] Implement `POST /v1/responses` with `store:false` and `stream:true` (live verification pending).
- [x] Implement bounded SSE completion/error handling and manual retry; offline tests pass.
- [ ] Verify browser sign-in, model access, real streamed reply and revocation with a consenting eligible account.

**Acceptance:** a consenting eligible Plus account requests “say hello” without manually entering an API key; account tokens never enter logs. Full platform tests later. **Do not silently substitute the paid API or reuse ChatGPT browser cookies.**

## Slice 4 — first real AI mutation (MVP wow moment)

- [x] Wire automatic context construction and the chat send path into AiChat; live request verification pending.
- [ ] Ask for `addWidget humidity-card` and parse typed mutation JSON.
- [x] Validate schema, component props, references and allowlisted permissions.
- [x] Wire accepted AI candidate to preview/confirmation and immutable commit; offline proposal-boundary tests pass.
- [ ] New view renders live humidity from Open-Meteo without AI.
- [ ] Restart disconnected; view persists. Failed/partial stream cannot mutate current state.

**Acceptance:** a user without coding skill makes a persistent GUI feature by describing it in Russian.

## Slice 5 — mobile and packaging

- [ ] Android phone/tablet build with responsive layout; no horizontal overflow.
- [ ] Windows packaging and Linux/macOS build checks.
- [ ] iOS compilation and real-device sign-in experiment on macOS + Xcode.
- [ ] Local export/import of app JSON only after versioned save is stable.

**Acceptance:** desktop and Android same `.mutyva.json` document; saved apps run offline. iOS support is a deliverable to verify, not an untested marketing claim.

## Priorities if time is tight

Never skip: official OAuth validity, safe JSON mutation validation, durable local persistence, real data and an offline restart. Cut: animations, advanced charting, huge palette, component sharing, SQLite generic browser, voice, drag-and-drop IDE and syncing.

## Definition of MVP completion

A recorded live test, from a **clean installation**, performs the entire sequence without developer tools: live data → ChatGPT sign-in & consent → AI-generated schema-valid mutation → preview → apply → restart disconnected → same working modified application → restore previous generation.
