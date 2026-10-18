# Implementation Roadmap — fastest path to MVP

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
- [x] Catalog of five known kinds with schema-typed properties and fixed source/binding descriptors.
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

- [ ] Implement official dynamic OAuth registration with host ID, PKCE, state and nonce.
- [ ] Verify OIDC ID token signature/issuer/audience/nonce, granted plan scope.
- [ ] Secure OS credential storage; refresh and disconnect.
- [ ] `GET /v1/models` under OAuth account; pick supported returned model slug.
- [ ] Stream a minimal `POST /v1/responses` with `store:false` and `stream:true`.
- [ ] Parse SSE until completed; show errors/retry for revoked session and plan usage limits.

**Acceptance:** a consenting eligible Plus account requests “say hello” without manually entering an API key; account tokens never enter logs. Full platform tests later. **Do not silently substitute the paid API or reuse ChatGPT browser cookies.**

## Slice 4 — first real AI mutation (MVP wow moment)

- [ ] Send current City Pulse document and safe component catalog to ChatGPT.
- [ ] Ask for `addWidget humidity-card` and parse typed mutation JSON.
- [ ] Validate schema, component props, references and allowlisted permissions.
- [ ] Preview accepted candidate and confirm; commit immutable generation.
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
