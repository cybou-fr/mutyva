# Implementation Roadmap — foundation MVP → useful local apps

**Product decision · 2026-10-19:** the normal experience is the running application plus **Чат с AI**. The host builds context, selects components and validates changes internally. JSON, palette controls, prompts and generations are developer tools, hidden behind Ctrl+Shift+D. Users describe a task and confirm its preview. See [14_CHAT_FIRST_UX.md](14_CHAT_FIRST_UX.md) for the primary workflow and [15_AI_CONNECTION.md](15_AI_CONNECTION.md) for implemented AI code and remaining verification.

**Workflow:** straight to `main`, small commits, docs kept in sync; avoid speculative release-number archaeology. Acceptance is measured by demonstrable behavior, not number of classes added.

**Qt 6.12 CI installation gate:** The public `aqtinstall` feed did not serve the Qt 6.12.0 MSVC metadata ([failure](https://github.com/cybou-fr/mutyva/actions/runs/39935175486)). CI now uses Qt's official installer with repository secrets `QT_EMAIL` and `QT_PW`, which must be configured before Windows Qt 6.12 verification can complete. Do not revert to Qt 6.8 for a passing badge; the Qt 6.12 acceptance gate remains open until MSVC build, full CTest and gallery pass.

**Qt baseline decision (2026-10-23):** MUTYVA requires **Qt 6.12.x**. CMake enforces the 6.12 series, and Windows CI is pinned to **6.12.0**. All prior green Windows Qt 6.8 results below remain historical until a Qt 6.12 rerun succeeds. Do not treat earlier Qt 6.8 artifact captures as Qt 6.12 evidence.

## Architectural review gate (2026-10-23)

**Status baseline and priorities:** [23_ARCHITECTURE_ASSESSMENT.md](23_ARCHITECTURE_ASSESSMENT.md) records the as-is context/component boundaries, 10 architecture-significant requirements (ASR-01…10) and risks (AR-01…09). [ADR registry](adr/README.md) documents choice/alternatives/consequences; [engineering workflow](24_ARCHITECTURE_WORKFLOW.md) documents evidence levels and Definition of Done. The standalone static CI now checks local ADR links and traceability, in addition to the component catalog; this is **not** a Qt 6.12 build.

**Immediate architecture fixes, already tracked (no separate duplicated backlog):**
- **AR-01 → R0-02 / P0:** if HEAD is missing but old snapshot/metadata exist, **fail closed**, never silently bootstrap a new current application. Confirm in negative/failure fixtures.
- **AR-02 → R0-03 / P0:** semantic preview must enumerate changed `model/filter/sort/style/options/message/source/icon/defaultCity` as well as action/parent and model changes. Show actual data/permission impact before Apply.
- **AR-03 → R0-05 / P0 blocking gate:** official Qt 6.12 installer requires GitHub `QT_EMAIL` and `QT_PW`; no build/CTest acceptance until configured and successfully run.
- **AR-04/05 → R1-07 and C1/C2 / P1:** characterize UI-state/record projection, then introduce the **smallest** host-owned typed selector/action seam; avoid speculative rewrite of `InteractionState`.
- **AR-06 → R0-04/C4a / P1:** harden current Open-Meteo size/deadlines/late replies before expanding sources.

**Architectural principle:** keep the local modular monolith and current native Qt/C++ architecture; extract modules **only** when an implemented vertical slice exercises a stable, tested interface. Do not increase component count to compensate for missing dataflow.

## Roadmap contract (reviewed 2026-10-23; baseline current main)

**Two acceptance levels, not a silent change of the original MVP:**

- **Foundation MVP (F-MVP):** the original City Pulse gate defined in [01_MVP_SPEC.md](01_MVP_SPEC.md): clean Windows launch, live public data, user-authorized ChatGPT mutation, safe preview/apply, offline restart, selectable earlier-generation restore. Slices 0–4 finish it; many core paths already exist, but it is **not yet accepted**.
- **Useful-app MVP (U-MVP):** a person creates an independent local task manager from a chat request, stores and edits structured records, then asks AI to add priority/due date/filtering without losing those records. Slices 5–7 are its additional acceptance gates.
- **Cross-platform release:** Windows distribution and Android real-device proof after the U-MVP is functional; Slice 8. Additional platforms are verification targets, not current claims.

**Execution order:** fix blocking durability/preview problems in Slice 2 first; close the remaining Foundation acceptance tests from Slices 0/4 and Qt 6.12 verification; finish Slice 5 storage/data migration → **R1C composition** → Slice 6 (multiple local apps) → Slice 7 (AI app creation) → Slice 8 (portability). Keep the working Windows ChatGPT integration; do not expand it while the runtime cannot create useful applications.

**Status conventions:** [x] means implemented or documented by current repo evidence, not independently audited; [ ] is not accepted. Each new work item below has an ID, a narrow implementation scope, and a pass/fail gate. Tests and user-visible behavior are the deliverable, not class count. Existing features remain regression requirements.

### Tracked scope and dependencies

| Stage | Dependency | User-visible outcome | Exit |
|---|---|---|---|
| R0 / Slice 2 + outstanding 0/4 | Existing code | Reliable previews, restore, weather demo | F-MVP gate |
| R1 / Slice 5 | R0 | Persistent typed records, safe CRUD and simple filtering | Data gate |
| R1C / Composition | R1 typed storage | One-way typed dataflow, typed actions, responsive layout and safe external snapshots | Composition gate |
| R2 / Slice 6 | R1C | Create/switch multiple separate local apps | Workspace gate |
| R3 / Slice 7 | R2 | Chat creates and evolves task manager from scratch | U-MVP gate |
| R4 / Slice 8 | R3 | Installable Windows + tested Android runtime | Release gate |

No MUTYVA backend, accounts, package registry, shared marketplace, cloud sync, arbitrary code execution, generic SQL supplied by AI, or on-device compilers. AI remains an optional designer; saved apps must not depend on inference.

## Slice 0 — bootstrap and live API (start immediately)

- [ ] Windows MSVC / **Qt 6.12.x** compiles with CMake using C++20 and QML; prior Qt 6.8 validation is historical only. Confirm in updated CI.
- [ ] City Search -> Open-Meteo geocoding -> forecast and air-quality HTTPS.
- [ ] Show loading, error, empty state, attribution and timestamps.
- [ ] UI usable from ~360 dp width to desktop width.
- [ ] Add a lightweight smoke test and save a screenshot for baseline.

**Delivered partially by `starter/`;** missing polish, user selection among homonymous cities and offline fixture mode are still work.

## Slice 1 — declarative UI contract

- [x] Validate against bundled `app.schema.json` before rendering, saving and restoring (checker supports this schema's vocabulary).
- [x] Catalog of 21 trusted kinds: five weather widgets, seven original general kinds and nine additional general/data UI kinds, per-kind properties, fixed weather bindings and typed local-data actions.
- [x] `WidgetHost` renders known kinds; titles are read from the declaration.
- [x] Separate weather service with typed read-only Q_PROPERTY data; fixed binding contract per kind, no expression evaluation.
- [x] Support `addWidget`, `removeWidget`, `updateProperty`, `moveWidget` with reference checks and ordered batch semantics.
- [x] Tests for malformed docs, unknown kinds, duplicate widget IDs, forbidden actions; offline QML preview/render tests.

**Acceptance path:** edit a declaration in the built-in JSON editor (or paste a hand-edited `.mutyva.json`), validate/preview and Apply without recompiling. File-picker import/export remains future work.

## Slice 2 / R0 — durability, trustworthy preview and baseline acceptance (NEXT)

**Existing:** file snapshots + metadata + HEAD via QSaveFile, QLockFile writer protection, stale-HEAD rejection, orphan filename avoidance, v1→v2 append-only migration, cancel/apply and saved restart. **New source candidate (unverified on Qt 6.12):** verified-ancestor history picker, fail-closed missing HEAD and full ordinary-user semantic diff. **Still missing:** explicit safe repair of lost/corrupt HEAD, broader injected-I/O fixtures and mandatory Qt 6.12 acceptance. Refer to [25_R0_RECOVERY_PREVIEW.md](25_R0_RECOVERY_PREVIEW.md).

- [x] Basic atomic snapshot and HEAD writes; conflicting writer rejected; corrupt HEAD blocks further writes without overwriting history.
- [x] Mutation preview, Apply/Cancel, restore after restart; basic previous-numeric-generation preview.
- [x] **R0-01 / P0 — confirmed history graph.** In AppDocument, record per-generation parent, UTC timestamp, short change description and canonical-document SHA-256. Preserve old files; migrate/interpret old metadata without overwriting originals. Rollback must follow a **confirmed parent pointer**, never generation-1. Handle orphan gap (HEAD 1, orphan 2, successful HEAD 3) and restored versions correctly. A restore creates a new generation with the selected historical content.
  - **R0-01 accepted on Windows:** parent/hash history, legacy compatibility and orphan rollback regressions passed in [Qt/MSVC CI (2026-10-23)](https://github.com/cybou-fr/mutyva/actions/runs/39933293033); see [18_GENERATION_METADATA.md](18_GENERATION_METADATA.md). Recovery UI remains R0-02.
- [ ] **R0-02 / P0 — recovery and rollback gates.** **IMPLEMENTED-UNVERIFIED slice (2026-10-23):** fail-closed absent HEAD with old generation/data, hash-linked ancestor-only history dialog and preview-by-ID, immutable Apply preserving SQLite model definitions, and positive/negative Qt tests landed; [design and test matrix](25_R0_RECOVERY_PREVIEW.md). **Still open:** user-approved repair when HEAD is missing, forced IO failure fixtures and verified Qt 6.12 regression.  Add history listing/selection UI (secondary screen). Verify hashes upon reading where present; fail closed on corruption. Test simulated interruption between snapshot and HEAD, partial/unwritable disk, stale writers, invalid parent and orphan recovery without silently selecting uncommitted content. No automatic deletion or rewriting of orphan snapshots.
- [ ] **R0-03 / P0 — user-readable semantic preview.** **IMPLEMENTED-UNVERIFIED slice (2026-10-23):** ordinary-mode summary now enumerates all widget properties, model/field additions, action/source context and unchanged permission/data boundary; Cancel/Apply tests and QML preview assertions added. [Contract](25_R0_RECOVERY_PREVIEW.md). **Still open:** Qt 6.12 UX/CTest proof, large-diff accessibility/visual checks.  Normal mode must list added/removed/reordered components, changed properties (including defaultCity), action targets, effects on visible app behavior, requested permissions and data-schema changes once supported. Developer JSON diff remains optional. No side effects in preview, and stale generations cannot Apply. Test ordinary mode, not only developer mode.
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

## Active implementation note — palette and typed records (2026-10-23)

The current code contains a **tested additive v2 typed-record contract** (RecordStore with app/model/record identity, record form/list, filters and sorting), nine additional trusted UI kinds, a Task Manager declaration and checked palette/schema/QML contracts. [Windows CI run 39933293033](https://github.com/cybou-fr/mutyva/actions/runs/39933293033) confirms MSVC/Qt 6.8 build, CTest **65 passed / 0 failed** (the optional image capture test skipped during the regular run), plus **a separate successful capture of 21 actual Qt PNGs**, published in the [gallery artifact](https://github.com/cybou-fr/mutyva/actions/runs/39933293033/artifacts/12491489800). The visually rendered palette has also been reviewed as a contact sheet. This is deliberately compatible with older v1/v2 snapshots and is **not** the planned independent v3 schema. The remaining R1 tickets stay open wherever v3 migration, error-injection coverage and complete E2E acceptance are still required. R2 multi-app and R3 AI creation remain unimplemented.

**Three-step palette delivery:** (1) typed records/CRUD implemented and Windows-tested; deeper v3 migration and failure gates remain, (2) nine additional Qt kinds compile and are rendered by the actual Qt 6.8 engine; mobile/manual design review remains, (3) reference, schema and QML consistency pass the CI contract test and **all 21 real PNGs have been captured** in the Windows CI gallery. Do not add more kinds before finishing the remaining R1 gates.

## Slice 5 / R1 — typed local data and practical UI actions

**Why this stage:** the previous runtime only persisted individual input/list widgets. A compatible typed CRUD slice now exists and passes Windows CI; full v3 migration, failure coverage and the R1 acceptance gate remain open. Adding more decorative widgets is not a substitute for verifying useful local applications.

- [ ] **R1-01 / P0 — versioned document contract.** Introduce an opt-in **schema v3** for app-owned data models while accepting v1/v2 and migrating by a new immutable generation. Start with model ID, stable field IDs, display label, field type (text, integer/number, boolean, date), required/default, a small fixed limit and strict validation. The AI never defines arbitrary SQL or executable expressions. A v3 document without data models remains valid.
- [x] **R1-02 / P0 — SQLite record store.** Extend local persistence with app/model/record identities; host generates immutable record IDs; implement typed create/read/update/delete with bound SQL parameters, transactions, per-app revision/concurrency checks and sensible size/count caps. Preserve existing widget_data schema and user entries. CRUD actions must not create interface generations. Database files/contents never enter AI prompts by default.
- [x] **R1-03 / P0 — minimal trusted components.** Add only **record-list**, **record-form** and an optional **record-detail** built from precompiled Qt/QML. Expose declarative model ID/field IDs and selected-record context; avoid arbitrary expression strings, handwritten SQL, QML/JS eval or dynamic native plugins. Reuse Text/Card/Stack/Row/Button where possible.
- [ ] **R1-04 / P0 — allowlisted actions.** Add typed record create/update/delete/select and explicit confirmation for destructive actions; validate model/field references and permitted side effects before preview/apply. No filesystem or network capabilities. Normal edits happen as runtime user actions, not AI calls.
- [ ] **R1-05 / P0 — non-destructive model evolution.** Add nullable/optional fields to an existing model while keeping all record IDs and values. Existing v3 documents/snapshots are immutable. Reject renames/removal/type changes that lack an explicit data migration contract; never silently drop user data. Show schema impacts in semantic diff.
- [ ] **R1-06 / P1 — basic filters and sort.** Typed equality/status and due-date filter plus stable order, enough for the second AI request in Slice 7; no general query language or full DataTable engine.
- [ ] **R1-07 / P0 — deterministic tests.** Fresh records, edit, delete/cancel, restart, stale writer, malformed values, schema migration v1/v2→v3, missing widget/model references, illegal action rejection, preview-only behavior, permissions, QML narrow width, and no user rows in AI context. Run offline in CTest.

**Gate R1:** using *manual validated declarations only* (no AI), build a task list with title/completed/priority/due-date fields, create and edit actual records, filter the list, restart offline with unchanged records and reject destructive schema mutations. This proves the runtime **before** making AI generate it.

**Design constraint:** use a small host-owned SQLite records table with validated typed JSON/columns; do not create arbitrary SQL tables or run user/model-supplied SQL in this phase. Avoid a general workflow language.

## R1C / Composition — the smallest useful AI application grammar (NEW, P0 before R2)

**Why:** 21 kinds are not enough if AI cannot make them **communicate**. In current v2, Notes work via two fixed list actions; record forms/lists share a model; weather bindings are fixed. No generic source-to-property selector, dynamic filter binding, host event routing, layout constraints or arbitrary remote API is present. Simply prompting AI to use imaginary `bind`/`onClicked` creates broken applications. See [20_COMPOSITION_CONTRACT.md](20_COMPOSITION_CONTRACT.md), [21_CONNECTOR_CONTRACT.md](21_CONNECTOR_CONTRACT.md) and [22_AI_DESIGN_PLAYBOOK.md](22_AI_DESIGN_PLAYBOOK.md).

**Preparatory static contract accepted:** [CI run 39955309544](https://github.com/cybou-fr/mutyva/actions/runs/39955309544) passed capability recipe/catalog/QML checks and full JSON Schema example validation. [Qt 6.12 run](https://github.com/cybou-fr/mutyva/actions/runs/39955309495) stopped at missing Qt Account secrets, before build; no Qt 6.12 runtime proof is claimed.

**Delivered now as preparation:** the bundled `runtime/designer-capabilities.json` guide is supplied in `DesignerContext`, with five small grounded recipes, role groupings, working wiring descriptions and an explicit unsupported/future capabilities list. CTest/static gate verifies catalog references, no conflicting support status and AI context inclusion. **This is truthful guidance, not implementation of the v3 graph.** Qt 6.12 end-to-end CI remains blocked on missing official-installer secrets; historical Qt 6.8 tests do not certify this new code.

- [ ] **C1 / P0 — typed one-way selectors and binding ports.** Introduce versioned host-declared output/input ports and compatible selectors for record selection → trusted Text/read-only view. Use stable widget/model IDs, typed null/empty/loading/error, app isolation, subscription lifetime and graph limits. Never allow string-evaluated expressions or QML supplied by AI. Gate: select/edit/delete a task and a linked label updates, including after restart and missing selection; bad type, missing ID and cycles rejected.
- [ ] **C2 / P0 — event-to-host-action route.** A registered event can dispatch exactly one typed host action with validated field sources; separate persistent record mutations from UI-only state changes; result/error events are bounded. Gate: trusted action fired once, duplicate click/stale revision/error has no partial write, invalid/unauthorized action rejected. The existing `record-form/list` and notes actions remain backwards-compatible.
- [ ] **C3 / P1 — accessible responsive layout primitives.** Extend only the known `stack/row/card/tabs` with a tiny token-based `gap/align/compact-versus-wide` contract. Qt owns measurement and accessible focus. Gate: 360dp + desktop screenshots/focus order; no overlap, horizontal overflow, invisible required controls or raw position expressions. Do not add Grid/Canvas as arbitrary model-controlled code.
- [ ] **C4a / P0 — normalized typed source snapshots.** Expose **existing** Open-Meteo data to a generic read-only port through registry-approved selectors with typed `loading/ready/empty/error/stale` status, bounded refresh/cancellation and source revisions; never send live payloads to AI. Gate: two views share the same source, no duplicate fetch, old response ignored and offline state honest.
- [ ] **C4b / P1 — second curated public HTTPS adapter (not arbitrary REST).** Only after C4a: one vetted read-only provider manifest, fixed allowlisted host and output schema, permission preview, strict HTTPS/redirect/network bounds, response limits, stale cache, clear attribution. Gate: offline HTTP fixture and SSRF/redirect/permissions tests; **no remote write or credentials**.
- [ ] **C5 / P0 — AI composer fixtures and semantic preview.** An app-intent test corpus (notes, tasks, weather, multi-panel) must produce the minimal *valid* local composition; unsupported dynamic bindings and arbitrary API requests yield truthful `unsupported`. Snapshot all four dimensions (layout, data, actions and external permissions) in the readable preview. Gate: 10 deterministic fixtures with no fake features, no side effects before Apply, no private record rows in AI context; the real ChatGPT gate remains R3.

**Composition acceptance (before R2):** with *manual schema-valid definitions*, build a local Task Manager in which form/list/selection/read-only detail share actual typed data; buttons invoke only allowlisted actions; layouts reflow from narrow to desktop; Open-Meteo provides a read-only normalized source. All app records survive restart and definition rollback. The host rejects fake bindings, uninstalled providers, secrets and new permission grants in an unapproved mutation. Only **then** automate this through ChatGPT.

**Version boundary:** current application schema remains **v2**; R1-01 must provide the opt-in v3 migration/compatibility before any C1–C4 new top-level ports, events, layout or source fields become executable. Documentation examples explicitly marked "future" are **not valid v2**. Avoid generic workflow engines, expression languages, third-party plugins, unrestricted REST and extra models.

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

Unrestricted REST/SQL connectors (distinct from vetted C4 read-only adapters), DataTable beyond the task demo, cross-app workflows, rich charting, free-form calculation/expression language, third-party plugin code, visual RAD designer, multi-provider inference, community sharing/sync, marketplace, filesystem automation and generation of executables. Add any of these only for a demonstrated user scenario and a documented security model.

## Execution tickets — first implementation batches

1. **Batch A, next:** R0-02 selected history/recovery and R0-03 readable preview including actions/models/permissions; keep Qt 6.12 CI access an explicit gate. Do not mix UI redesign with storage refactor.
2. **Batch B:** R0-04 weather bounds/fixtures; R0-05 Qt 6.12 CI/license and clean-install verification. Finish R0-06 clean Windows acceptance.
3. **Batch C:** R1-01 schema v3 migration/compatibility; harden already landed R1-02 SQLite CRUD and R1-07 transaction/error-injection tests. Gate without AI.
4. **Batch D:** finish R1-04/R1-05/R1-06 contract acceptance and C1 typed selector → UI, then C2 typed event/action and C3 responsive tokens.
5. **Batch E:** C4a Open-Meteo normalized snapshot and vetted C4b only if required; C5 AI fixture quality; then R2-01 through R2-04 independent app catalog/isolation.
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
