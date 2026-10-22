# Implementation Roadmap — foundation MVP → useful local apps

**Product decision · 2026-10-19:** the normal experience is the running application plus **Чат с AI**. The host builds context, selects components and validates changes internally. JSON, palette controls, prompts and generations are developer tools, hidden behind Ctrl+Shift+D. Users describe a task and confirm its preview. See [14_CHAT_FIRST_UX.md](14_CHAT_FIRST_UX.md) for the primary workflow and [15_AI_CONNECTION.md](15_AI_CONNECTION.md) for implemented AI code and remaining verification.

**Workflow:** straight to `main`, small commits, docs kept in sync; avoid speculative release-number archaeology. Acceptance is measured by demonstrable behavior, not number of classes added.

## Roadmap contract (reviewed 2026-10-23; baseline main 4ff67fb)

**Two acceptance levels, not a silent change of the original MVP:**

- **Foundation MVP (F-MVP):** the original City Pulse gate defined in [01_MVP_SPEC.md](01_MVP_SPEC.md): clean Windows launch, live public data, user-authorized ChatGPT mutation, safe preview/apply, offline restart, selectable earlier-generation restore. Slices 0–4 finish it; many core paths already exist, but it is **not yet accepted**.
- **Useful-app MVP (U-MVP):** a person creates an independent local task manager from a chat request, stores and edits structured records, then asks AI to add priority/due date/filtering without losing those records. Slices 5–7 are its additional acceptance gates.
- **Cross-platform release:** Windows distribution and Android real-device proof after the U-MVP is functional; Slice 8. Additional platforms are verification targets, not current claims.

**Execution order:** fix blocking durability/preview problems in Slice 2 first; close the remaining Foundation acceptance tests from Slices 0/4; then build Slice 5 → 6 → 7 → 8. Keep the working Windows ChatGPT integration; do not expand it while the runtime cannot create useful applications.

**Status conventions:** [x] means implemented or documented by current repo evidence, not independently audited; [ ] is not accepted. Each new work item below has an ID, a narrow implementation scope, and a pass/fail gate. Tests and user-visible behavior are the deliverable, not class count. Existing features remain regression requirements.

### Tracked scope and dependencies

| Stage | Dependency | User-visible outcome | Exit |
|---|---|---|---|
| R0 / Slice 2 + outstanding 0/4 | Existing code | Reliable previews, restore, weather demo | F-MVP gate |
| R1 / Slice 5 | R0 | Persistent typed records, safe CRUD and simple filtering | Data gate |
| R2 / Slice 6 | R1 | Create/switch multiple separate local apps | Workspace gate |
| R3 / Slice 7 | R2 | Chat creates and evolves task manager from scratch | U-MVP gate |
| R4 / Slice 8 | R3 | Installable Windows + tested Android runtime | Release gate |

No MUTYVA backend, accounts, package registry, shared marketplace, cloud sync, arbitrary code execution, generic SQL supplied by AI, or on-device compilers. AI remains an optional designer; saved apps must not depend on inference.

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

## Slice 2 / R0 — durability, trustworthy preview and baseline acceptance (NEXT)

**Existing:** file snapshots and HEAD via QSaveFile, QLockFile writer protection, stale-HEAD rejection, orphan filename avoidance, v1→v2 append-only migration, cancel/apply and saved restart. **Still missing:** explicit parent/hash metadata, selectable history, recovery path and ordinary-user semantic diff. These are not checked off merely because snapshots exist.

- [x] Basic atomic snapshot and HEAD writes; conflicting writer rejected; corrupt HEAD blocks further writes without overwriting history.
- [x] Mutation preview, Apply/Cancel, restore after restart; basic previous-numeric-generation preview.
- [ ] **R0-01 / P0 — confirmed history graph.** In AppDocument, record per-generation parent, UTC timestamp, short change description and canonical-document SHA-256. Preserve old files; migrate/interpret old metadata without overwriting originals. Rollback must follow a **confirmed parent pointer**, never generation-1. Handle orphan gap (HEAD 1, orphan 2, successful HEAD 3) and restored versions correctly. A restore creates a new generation with the selected historical content.
- [ ] **R0-02 / P0 — recovery and rollback gates.** Add history listing/selection UI (secondary screen). Verify hashes upon reading where present; fail closed on corruption. Test simulated interruption between snapshot and HEAD, partial/unwritable disk, stale writers, invalid parent and orphan recovery without silently selecting uncommitted content. No automatic deletion or rewriting of orphan snapshots.
- [ ] **R0-03 / P0 — user-readable semantic preview.** Normal mode must list added/removed/reordered components, changed properties (including defaultCity), action targets, effects on visible app behavior, requested permissions and data-schema changes once supported. Developer JSON diff remains optional. No side effects in preview, and stale generations cannot Apply. Test ordinary mode, not only developer mode.
- [ ] **R0-04 / P1 — real data and layout hardening.** In WeatherService bound reply size/deadlines and response validation; distinguish missing measurements from zero; avoid picking an ambiguous city silently; add deterministic HTTP fixtures for HTTP 429/500, timeout, malformed/empty API and late results. Verify 360dp and desktop layouts in Qt offscreen smoke; keep Open-Meteo credit.
- [ ] **R0-05 / P1 — automation/legal baseline.** Add GitHub Actions Windows configure/build/CTest (no interactive OAuth in CI), then Linux compile/smoke when viable; choose and commit an explicit source license, review brand assets/Qt/Open-Meteo attribution, publish reproducible Windows build instructions. Record screenshots or a short test log from a clean install.
- [ ] **R0-06 / P0 — Foundation end-to-end gate.** From clean Windows state: weather works without sign-in; consented real ChatGPT request changes humidity/default city; normal user sees meaningful diff and confirms; restart **without ChatGPT** preserves the configuration; history chooses an older verified generation and restores it as a new one. Include successful and rejected/cancelled cases. Real-account flow is opt-in and not required for every CI run.

**Deliverable:** a trustworthy small AI-mutable Qt application, not just a working JSON editor.

**Suggested small commits:** history parent regression test → metadata and historical restore → preview diff → weather fixtures/bounds → CI/license/clean-install evidence. Keep scope separate so failures are bisectable.

## Slice 3 — Sign in with ChatGPT (critical feasibility gate)

Preparation for AI integration is now implemented locally: shared component catalog with property/binding/action metadata, trusted designer instructions, bounded inspectable context and a checked proposal/clarification/unsupported response envelope. See `11_AI_DESIGN_CONTRACT.md`. Windows ChatSession and AiChat now implement the first browser sign-in and Responses path; a real-account Windows suite passed (doc16); persistent sessions and refresh remain pending.

General palette v2 is implemented with parent containers, append-only v1 migration, an interactive Notes composition and separate persistent SQLite input/list data with transactional actions and stale-writer protection. See `12_GENERAL_PALETTE.md` and `13_LOCAL_DATA.md`. Generic business records and DataTable remain future work.

- [x] Implement first Windows dynamic OAuth registration with host ID, PKCE, state and nonce (real Windows live suite passed; doc16).
- [x] Implement Windows RS256/CNG verification of ID token and granted plan scope, with offline negative tests (real Windows live suite passed; doc16).
- [ ] Secure OS credential storage; refresh and disconnect.
- [x] Implement `GET /v1/models` under OAuth account and automatic visible-model selection (real Windows live suite passed; doc16).
- [x] Implement `POST /v1/responses` with `store:false` and `stream:true` (real Windows live suite passed; doc16).
- [x] Implement bounded SSE completion/error handling and manual retry; offline tests pass.
- [x] Verify browser sign-in, model access and real streamed reply with a consenting account (doc16).
- [ ] Verify revocation and renewable persistent sessions.

**Acceptance:** a consenting eligible Plus account requests “say hello” without manually entering an API key; account tokens never enter logs. Full platform tests later. **Do not silently substitute the paid API or reuse ChatGPT browser cookies.**

## Slice 4 — first real AI mutation (MVP wow moment)

- [x] Wire automatic context construction and the chat send path into AiChat; real Windows live request passed (doc16).
- [x] Ask for `addWidget humidity-card` and parse typed mutation JSON (real live suite).
- [x] Validate schema, component props, references and allowlisted permissions.
- [x] Wire accepted AI candidate to preview/confirmation and immutable commit; offline proposal-boundary tests pass.
- [ ] New view renders live humidity from Open-Meteo without AI.
- [x] Recreate AppDocument after a real AI mutation without another inference request; saved definition persists. Failed/partial stream protection has offline tests.

**Acceptance:** a user without coding skill makes a persistent GUI feature by describing it in Russian.

## Slice 5 / R1 — typed local data and practical UI actions

**Why next:** today general widgets persist input/list values, but the host has no generic record collections, record relations, CRUD forms or filters. Adding more weather cards does not prove the product promise.

- [ ] **R1-01 / P0 — versioned document contract.** Introduce an opt-in **schema v3** for app-owned data models while accepting v1/v2 and migrating by a new immutable generation. Start with model ID, stable field IDs, display label, field type (text, integer/number, boolean, date), required/default, a small fixed limit and strict validation. The AI never defines arbitrary SQL or executable expressions. A v3 document without data models remains valid.
- [ ] **R1-02 / P0 — SQLite record store.** Extend local persistence with app/model/record identities; host generates immutable record IDs; implement typed create/read/update/delete with bound SQL parameters, transactions, per-app revision/concurrency checks and sensible size/count caps. Preserve existing widget_data schema and user entries. CRUD actions must not create interface generations. Database files/contents never enter AI prompts by default.
- [ ] **R1-03 / P0 — minimal trusted components.** Add only **record-list**, **record-form** and an optional **record-detail** built from precompiled Qt/QML. Expose declarative model ID/field IDs and selected-record context; avoid arbitrary expression strings, handwritten SQL, QML/JS eval or dynamic native plugins. Reuse Text/Card/Stack/Row/Button where possible.
- [ ] **R1-04 / P0 — allowlisted actions.** Add typed record create/update/delete/select and explicit confirmation for destructive actions; validate model/field references and permitted side effects before preview/apply. No filesystem or network capabilities. Normal edits happen as runtime user actions, not AI calls.
- [ ] **R1-05 / P0 — non-destructive model evolution.** Add nullable/optional fields to an existing model while keeping all record IDs and values. Existing v3 documents/snapshots are immutable. Reject renames/removal/type changes that lack an explicit data migration contract; never silently drop user data. Show schema impacts in semantic diff.
- [ ] **R1-06 / P1 — basic filters and sort.** Typed equality/status and due-date filter plus stable order, enough for the second AI request in Slice 7; no general query language or full DataTable engine.
- [ ] **R1-07 / P0 — deterministic tests.** Fresh records, edit, delete/cancel, restart, stale writer, malformed values, schema migration v1/v2→v3, missing widget/model references, illegal action rejection, preview-only behavior, permissions, QML narrow width, and no user rows in AI context. Run offline in CTest.

**Gate R1:** using *manual validated declarations only* (no AI), build a task list with title/completed/priority/due-date fields, create and edit actual records, filter the list, restart offline with unchanged records and reject destructive schema mutations. This proves the runtime **before** making AI generate it.

**Design constraint:** use a small host-owned SQLite records table with validated typed JSON/columns; do not create arbitrary SQL tables or run user/model-supplied SQL in this phase. Avoid a general workflow language.

## Slice 6 / R2 — independent local applications and workspaces

- [ ] **R2-01 / P0 — AppCatalog and selection.** List locally installed app manifests and titles; create a blank or bundled template with a new stable ID; open, switch and rename independently. Existing City Pulse data/history must migrate or remain addressable without changing the working current install.
- [ ] **R2-02 / P0 — per-app isolation.** App ID scopes generations, records, UI state and permissions; app A operations cannot change B. Keep global ChatGPT sign-in separate from app identities. Reopening an app must not depend on network or AI.
- [ ] **R2-03 / P1 — safe local export/import.** Import/export only declarative definitions by explicit action, validated on import. Do not export private records or credentials by default; import uses a new ID on collision and never overwrites existing apps silently. Package format and arbitrary asset handling are out of scope.
- [ ] **R2-04 / P0 — tests and UX.** First launch City Pulse or clear Home → New App → create local task manager from bundled valid definition → switch to City Pulse → switch back → both configurations and data persist. Include collision, corrupt catalog, interrupted creation and app isolation tests.

**Gate R2:** at least City Pulse and Task Manager coexist, with separate record data, histories and no accidental cross-app mutations; users can create/open/switch them with no developer controls.

## Slice 7 / R3 — AI creates an app, then evolves it safely

- [ ] **R3-01 / P0 — create-app contract.** Extend designer response with one **createApplication** proposal type, separate from existing mutate-app response. Request contains user intent, available local palette/schema and no private rows. AI returns a full validated v3 declaration and app name; host assigns/validates local app identity and capabilities. Do not ask AI to write QML/C++/SQL.
- [ ] **R3-02 / P0 — preview and atomic creation.** Preview new app UI, data model, actions and explicit side effects *before* creating its catalog entry or database. Cancel leaves no new app. Apply creates app/history/data atomically as far as local persistence permits, with recovery for interrupted creation; any I/O failure preserves previous apps.
- [ ] **R3-03 / P0 — reliable incremental mutation.** AI edits the existing app model/UI through the same checked mutation pipeline. Adding priority/due date/filter must preserve all preexisting task rows and their stable IDs; incompatible data operations must be rejected or require a separately reviewed migration.
- [ ] **R3-04 / P0 — bounded quality fixtures.** Deterministic offline AI response fixtures for new task manager, unsupported action, schema conflict, wrong app/generation, cancellation and partial SSE. Verify only trusted kinds and typed actions are accepted. At least one opt-in real Windows ChatGPT E2E run; never conflate mocked fixtures with live proof.
- [ ] **R3-05 / P1 — plain-language everyday UX.** Home prompt "Создай менеджер задач" → human-understandable new-app preview → Apply → usable app; in-app prompt "Добавь приоритет, срок и фильтр просроченных" → semantic diff → Apply. No mandatory JSON/schema/model selection.

**Gate R3 / Useful-app MVP:** complete recorded clean-install scenario: chat creates a working Task Manager; user adds three or more tasks, edits one and completes another; second request adds priority, due date and filtering; task identities and values survive; app restarts and works **with ChatGPT disconnected**; another local app is unchanged; an earlier verified UI generation can be restored without reverting user data. Unsupported requests fail honestly.

## Slice 8 / R4 — portable runtime, session lifecycle and distribution

- [ ] **R4-01 / P0 — Windows distribution.** Reproducible release package including QML/Qt SQL plugins, first-run/error states, meaningful privacy/attribution notices; cold-install smoke and normal-user workflow from Slice 7.
- [ ] **R4-02 / P0 — Android real device.** Build and run the native Qt/QML app on a real Android phone/tablet; test responsive layout, local data migration, offline app execution, import of the same validated app declaration and storage integrity. Port the official sign-in only after platform-compatible OIDC/redirect verification; do not label AI sign-in supported until tested.
- [ ] **R4-03 / P1 — secure ChatGPT lifecycle.** Native OS protected credential storage, token refresh/rotation, explicit disconnect/revocation tests and session recovery. Do not persist access/refresh tokens in QSettings, app documents or SQLite app data. Keep Windows real-account regression opt-in.
- [ ] **R4-04 / P1 — further platforms.** Linux/macOS compile and packaging smoke; iOS feasibility/security/sign-in evaluation on actual Apple toolchain/device. Advertise only platforms with proven gates.
- [ ] **R4-05 / P1 — import/export recovery.** Complete safe local backup/restore for app definition and opt-in user records after app and record schemas are stable; exclude credentials and never silently overwrite data.

**Gate R4:** same saved declarative app runs offline on Windows and real Android with equivalent behavior and intact data; distributions install cleanly. ChatGPT mutations on Android require a separate verified auth gate.

## Later research (explicitly NOT a U-MVP dependency)

Arbitrary REST/SQL connectors, DataTable beyond the task demo, cross-app workflows, rich charting, free-form calculation/expression language, third-party plugin code, visual RAD designer, multi-provider inference, community sharing/sync, marketplace, filesystem automation and generation of executables. Add any of these only for a demonstrated user scenario and a documented security model.

## Execution tickets — first implementation batches

1. **Batch A, immediately:** R0-01 regression exposing numeric rollback across an orphan, then safe parent-based history; R0-03 readable preview including defaultCity. Do not mix UI redesign with storage refactor.
2. **Batch B:** R0-02 selected restore and corruption/crash tests; R0-04 weather bounds/fixtures; R0-05 CI/license. Finish R0-06 clean Windows acceptance.
3. **Batch C:** R1-01 versioned typed model + migration; R1-02 SQLite CRUD; R1-07 unit/transaction tests. Gate without AI.
4. **Batch D:** R1-03 components and R1-04 actions; R1-05 safe schema addition; R1-06 filters; task manager manual end-to-end.
5. **Batch E:** R2-01 through R2-04 app catalog, isolation, switching and tests.
6. **Batch F:** R3-01 through R3-05 create-app contract, preview, actual ChatGPT Task Manager create→mutate→offline scenario.
7. **Batch G:** R4 only after U-MVP gate; begin Windows package + Android offline proof, then protected session work as needed.

Each batch ships through small reviewable commits, linked offline tests, and a short current-state update to README, [01_MVP_SPEC.md](01_MVP_SPEC.md), [04_DECLARATIVE_RUNTIME.md](04_DECLARATIVE_RUNTIME.md), [07_TESTING_SECURITY.md](07_TESTING_SECURITY.md) or [08_PLATFORMS_RELEASE.md](08_PLATFORMS_RELEASE.md) **only when their contracts actually change**. Do not pre-claim completed gates.

## Priorities if time is tight

**Never cut:** a validated data model, typed CRUD, safe migration without record loss, meaningful normal-user preview, atomic persistence/recovery, app isolation, real create-app+mutate-app ChatGPT proof, offline restart and negative tests.

**Defer:** advanced history analytics, visual editor, polished animations, general SQL connectors, external REST integration beyond demo, huge widget palette, voice, marketplace, synchronized accounts, generated executable code and unverified platforms.

**Focus invariant:** AI designs and proposes; the host validates and previews; the user confirms; the trusted Qt runtime executes locally. Domain records are not interface history and are not automatically disclosed to the model.

## Definition of done

**Foundation MVP:** on clean Windows, complete City Pulse live → user-authorized ChatGPT mutation → safe readable preview → Apply → restart disconnected → restore *verified* prior generation, with failure/cancel tests and no loss of saved state.

**Useful-app MVP:** on clean Windows, create Task Manager through chat → store and edit real tasks → AI adds priority/due date/filter → validate and preview → Apply → restart offline with intact tasks → open a second independent app → restore an older UI generation without changing user records. The same scenario must have deterministic offline regression tests plus a separately documented opt-in real-account test.

**Release:** Windows + Android real-device smoke; verified local persistence and app portability. Do not mark cross-platform support, persistent credentials or license compliance complete until validated.
