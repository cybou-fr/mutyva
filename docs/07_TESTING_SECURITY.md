# Security, Tests and Quality Gates

## Threat boundary

1. **ChatGPT is a proposal engine, not a code executor.** All model output is untrusted and checked. No `Qt.createQmlObject` for model output, dynamic QML/JS `eval`, shell commands, arbitrary SQL or native plugin loading.
2. **API data is untrusted.** Validate HTTP status, JSON types, lengths, value ranges and missing fields. Never trust instructions embedded in public API response strings.
3. **Browser OAuth is a privileged flow.** PKCE S256, state, nonce, exact loopback callback, ID-token verification, granted scope check, secure credentials and refresh token rotation.
4. **Preview changes before Apply.** Permissions are granted separately, with domain/resource scoping. A UI mutation cannot silently change network access or delete data.
5. **Data minimization.** Only source schemas, public metadata and approved sample context go to ChatGPT. Do not send private DB rows or credentials without explicit permission.
6. **Offline state.** Cache with timestamp; distinguish last-known data from real-time data. A working saved app does not need AI, but any remote data naturally requires network access.

## Test matrix

| Test | Expected |
|---|---|
| Start without sign-in | Public API demo runs; design-by-AI disabled, with explanation |
| Valid city | Weather/air card renders and updates |
| Nonexistent city | Human-readable empty state |
| Network down | No crash or frozen UI; clear cached/stale state |
| HTTP 429 / 500 | Error/retry behavior, no infinite tight loop |
| Malformed JSON from API | Data rejected; error shown |
| Mutation unknown kind | Reject; no new generation |
| Duplicate widget ID | Reject; no new generation |
| Unapproved permission | Block or request explicit approval |
| Cancel preview | Zero persisted changes |
| Apply valid mutation | New immutable generation; UI updated |
| Restart offline | Same UI configuration, no AI call |
| Restore earlier generation | New generation with old UI; external DB unaffected |
| OAuth wrong state/nonce | Reject session |
| OAuth scope missing | Do not infer AI entitlement |
| SSE connection drops | Discard partial proposal |
| App data folder read-only | Show save failure, keep previous HEAD |
| Narrow/mobile screen | Controls reachable by touch and keyboard |

## Automated suites to add

Implemented in `tests/RuntimeTest.cpp`: bundled-schema/catalog checks, document rejection cases, ordered four-operation batches, all-or-nothing rejection, permission gate, preview/cancel/apply, saved restart, writer lock/stale HEAD, orphan snapshot preservation, corrupt snapshot rejection, and offline QML title/preview rendering at desktop and narrow widths. Run through CTest; no live API or OAuth is used by these tests.

Remaining suites and expansion:

General-palette coverage adds v1→v2 migration without overwriting the source snapshot, parent/type/reference/cycle/depth checks, subtree deletion, reparenting, per-kind properties, append/clear limits, preview action blocking, local-data exclusion from AI context/history, nested QML interaction and narrow-window rendering. SQLite coverage verifies restart persistence, app isolation, retained data after widget removal/restoration, stale-writer rejection, write-lock handling, transaction rollback, corrupt physical/logical databases and storage limits. Tests use temporary databases.

- `AppDocumentTest` — parse, canonical serialize, stable IDs, unknown/duplicate references.
- `MutationEngineTest` — allowable ops, props, permissions, rollback atomicity.
- `SseDecoderTest` — fragmented chunks, multi-line event payload, completion and error events.
- `AuthFlowTest` — callback state, PKCE, OIDC issuer/audience/signature, token refresh rotation.
- `WeatherConnectorTest` — replay HTTP fixtures; no live internet required for CI.
- QML smoke tests for responsive views and keyboard operation.

## Safety checklist for generated custom components

For MVP only compositions of known kinds; include an allowlist of kinds, typed props, per-kind capabilities, unique IDs, dependency cycle detection and configurable resource limits. Sandbox arbitrary code is a post-MVP research problem and has iOS distribution constraints.

## Logging and telemetry

Default to local diagnostic logs with redaction and no remote telemetry. Do not store OAuth access/refresh tokens, raw authorization codes, cookies, full prompts containing user secrets or entire API payloads in crash reports.

## Licence + attribution

Qt licensing must be reviewed for chosen Qt modules and distribution model. Open-Meteo free plan is **noncommercial**, CC BY 4.0 attribution and subject to rate limits. Include provider credit and links in the demo; do not treat publicly readable API as unrestricted commercial usage. OpenAI plan-usage eligibility depends on the user's account and current program conditions.

## First Windows AI integration

See [15_AI_CONNECTION.md](15_AI_CONNECTION.md). Offline tests cover CNG RSA signature verification and JWT claim failures, PKCE/callback validation, chunked SSE completion/errors/limits and chat proposal-to-preview behavior. Live browser auth, token exchange, model access, reply and isolated mutation/apply/restart passed on one Windows account on 2026-10-19 (doc16). Revocation and other platforms remain unverified. Tokens and chat are memory-only; persistence/refresh/multiple accounts require further implementation.

Model discovery also has an injectable offline network test covering a 600 KiB catalog through Responses and preview, 401/429/network failures, malformed/empty/unexpected catalogs, the 4 MiB bound and cancellation. Diagnostic output contains only fixed stage/reason enums and numeric metadata; never server bodies or credentials.

## Human-authenticated live suite

See [16_LIVE_AI_TESTS.md](16_LIVE_AI_TESTS.md). The standalone live-ai-tests executable opens real browser sign-in, then automatically exercises a nonmutating reply and an AI mutation through temporary preview/apply/restart. Its atomic JSON report distinguishes waiting/running from passed/failed/cancelled. One sign-in covers the suite; no screenshots or manual prompts are needed. The user's live generations/data are not used. Offline harness tests remain separate from evidence of a real live pass.

Default-city coverage verifies validation/catalog/context, preview cancellation, apply/restart/rollback, QML preview input, no candidate-city HTTP requests before Apply, preservation of manual search on unrelated document changes and late-response isolation. The extended real-account suite passed the Poitiers mutation/apply/restart scenario; see [17_DEFAULT_CITY.md](17_DEFAULT_CITY.md).

## Composition graph / external data test gates (planned R1C)

The bundled AI `compositionGuide` is a **fact sheet**, not elevated instructions: a candidate that matches a recipe must still satisfy current source schemas, permissions and implementation. The static `palette-contract` test now checks recipe kind references, capability status contradictions, inclusion in AI context and CMake resources. [Standalone contract CI](https://github.com/cybou-fr/mutyva/actions/runs/39955309544) passed JSON Schema and AI recipe checks without Qt. Qt 6.12 CTest remains blocked until official installer credentials are configured; do not claim a successful run on 6.12 yet.

When C1–C4 are implemented, add **negative** fixtures for missing/mismatched ports, cycles, over-limit fanout, cross-app selectors, selection deleted mid-flight, stale writer, duplicate event, failed transaction, revoked permissions, preview-no-side-effects and untrusted arbitrary URLs. Require a read-only connector fixture for timeout, HTTP 429/500, malformed/oversize payloads, HTTP redirects and SSRF blocking, stale input revision, offline cache and correct timestamp attribution. All input/action/binding schemas must be versioned; renderer never evaluates AI-authored scripts or unknown names. See [20](20_COMPOSITION_CONTRACT.md) and [21](21_CONNECTOR_CONTRACT.md).
