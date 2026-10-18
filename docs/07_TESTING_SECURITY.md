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
