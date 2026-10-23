# Qt/C++ Architecture

**Product decision · 2026-10-19:** the normal experience is the running application plus **Чат с AI**. The host builds context, selects components and validates changes internally. JSON, palette controls, prompts and generations are developer tools, hidden behind Ctrl+Shift+D. Users describe a task and confirm its preview. See [14_CHAT_FIRST_UX.md](14_CHAT_FIRST_UX.md) for the primary workflow and [15_AI_CONNECTION.md](15_AI_CONNECTION.md) for implemented AI code and remaining verification.

**Authoritative as-is architecture and traceable risks:** [23_ARCHITECTURE_ASSESSMENT.md](23_ARCHITECTURE_ASSESSMENT.md) (source-grounded C4 views, ASRs, risks). **Decision alternatives:** [ADRs](adr/README.md). **Delivery discipline:** [24_ARCHITECTURE_WORKFLOW.md](24_ARCHITECTURE_WORKFLOW.md). This older architecture page mixes current and target responsibilities; prefer doc23 for exact implementation status.

## Current versus target (mandatory distinction)

The **implemented Qt 6.12 target baseline** is enforced in CMake (Windows Qt 6.12 CI needs official-installer secrets). Compiled host classes today are `AppDocument`, `DeclarativeRuntime`, `InteractionState`, `LocalDataStore`, `RecordStore`, `WeatherService`, `DesignerContext`, `AiChat` and `ChatSession`. The diagram/table below describe the **target logical responsibilities**, not a claim that each row is an implemented standalone class. Actual component kinds and restrictions are in [19_COMPONENT_REFERENCE.md](19_COMPONENT_REFERENCE.md).

See [20_COMPOSITION_CONTRACT.md](20_COMPOSITION_CONTRACT.md) for typed cross-component bindings, event/action routing, responsive tokens and app-scoped state; [21_CONNECTOR_CONTRACT.md](21_CONNECTOR_CONTRACT.md) for vetted external datasets; [22_AI_DESIGN_PLAYBOOK.md](22_AI_DESIGN_PLAYBOOK.md) for the grounded capability guide now supplied to the AI designer.

## Runtime layers

```mermaid
flowchart TB
 U[Text request / Visual editing] --> A[AiDesigner - only on change]
 A --> P[MutationProposal]
 P --> V[Schema + semantic + permission validation]
 V --> X[Preview / Apply]
 X --> G[GenerationStore immutable snapshots]
 G --> D[AppDocument]
 D --> R[Qt QML Declarative Renderer]
 C[Trusted Component Registry] --> R
 E[Action + Binding Engine] --> R
 S[Open-Meteo REST / local SQLite] --> E
```

## Target modules, concrete responsibilities

| Module | Stack | Contract |
|---|---|---|
| `MutyvaShell` | QML Quick Controls | Responsive shell, chat/design sheet, previews, generation browser |
| `AppDocument` | C++/JSON | Canonical validated description, stable IDs, immutable structures |
| `ComponentRegistry` | C++ types and QML components | Look up `kind`, property schema, capabilities, target compatibility |
| `UiRenderer` | QML Loader + prebuilt types | Renders known components only, no string-to-QML eval |
| `WeatherConnector` | Qt Network | Public HTTPS API calls, parse/normalize, retries and errors |
| `DataStore` | Qt SQL + SQLite | Local bookmarks/cache/document catalog in future MVP slices |
| `BindingEngine` | C++ | Read-only selectors and typed state subscriptions, no eval |
| `ActionEngine` | C++ | Allowlisted actions: refresh, search, select, set local value |
| `MutationEngine` | C++ | Validate, construct candidate, preview, commit or discard |
| `GenerationStore` | SQLite+filesystem | Immutable snapshots, atomic HEAD, restore as new generation |
| `ChatGPTAuth` | Qt NetworkAuth + OIDC validator | SIWC PKCE, host/client IDs, token exchange, refresh and secure storage |
| `AiDesigner` | Qt Network / SSE | Account-specific model choice, streaming Responses, typed mutation proposal |

## Important dependency boundaries

- `UiRenderer` **must not know about** OAuth or OpenAI. It only sees a validated `AppDocument`.
- AI output **must not call arbitrary native functions**. It can request only enumerated mutation operations.
- `ComponentRegistry` enumerates known kinds and property schemas, so unsupported kinds fail before rendering.
- `DataConnectors` own all I/O. QML never sees access tokens and never concatenates untrusted SQL.
- `GenerationStore` persists definitions and references; external business data is independent and is not snapshotted by GUI rollback.
- Account sign-in is optional for normal execution. This is not a local MUTYVA login.

## Source layout for intended full MVP

```text
src/
  app/ AppDocument, GenerationStore, MutationEngine
  components/ ComponentRegistry, PropertySchema
  data/ WeatherConnector, RestConnector, LocalStore
  ai/ ChatGPTAuth, OidcVerifier, AiDesigner, SseDecoder
  security/ PermissionGate, ProtectedCredentialStore
qml/
  shell/ Landing, Workspace, DesignSheet, MutationPreview, History
  components/ CitySearch, WeatherSummary, ForecastList, AirQuality, Humidity
schemas/
  app.schema.json, mutation.schema.json
examples/
  city-pulse.mutyva.json
```

## Data flow (live weather)

User city -> geocoding GET -> coordinates -> parallel forecast GET + air-quality GET -> normalized typed CityWeather state -> Q_PROPERTY signals -> QML components. Search and display do **not** invoke ChatGPT.

## Data flow (AI mutation)

Prompt + approved app context + available kinds -> authenticated `POST /v1/responses` with streaming -> parse final assistant proposal -> JSON Schema validation -> semantic validation -> preview -> Apply -> atomic snapshot generation -> renderer incremental change.

## Local state, persistence and portability

Full MVP uses SQLite catalog with `apps`, `generations`, `component_installations`, `local_settings`, `api_cache` tables plus immutable JSON blobs stored locally. Use content hash as an integrity check (not security signing). Keep all credentials in OS secure storage, separately from exportable app packages. Use `QStandardPaths::AppDataLocation`; do not hardcode `%APPDATA%` or `$HOME`.

## Cross-platform nuance

`QOAuth2AuthorizationCodeFlow` helps with PKCE, but a production OIDC verifier must validate tokens and a protected credential store must be implemented; “Qt has OAuth” is not equal to “we have secure ChatGPT auth”. Native loopback callback on `127.0.0.1` must be proven on each mobile OS. See `03_CHATGPT_AUTH.md` and `08_PLATFORMS_RELEASE.md`.

## Working source baseline (2026-10-23)

The repository implements 21 trusted Qt/QML component kinds, one running City Pulse app, immutable definition generations with verified ancestry, typed local SQLite record CRUD, static model filters/sort, narrow/wide row behavior, five compiled Open-Meteo views, browser-based ChatGPT editing and an inspectable mutation preview path. A Task Manager example runs **within that original app**; it is **not** independent multi-app creation. The Qt 6.8 CI passed the existing baseline, but **Qt 6.12 validation remains blocked on Qt Account secrets**, not assumed passed.

**Wiring today is limited:** two generic list actions, host-owned record-form/list sharing a model and fixed Open-Meteo city/weather bindings. `select` cannot dynamically filter `record-list` and generic `text` cannot bind to arbitrary model or connector fields. DesignerContext now supplies a machine-readable recipe/capability guide to prevent false compositions, without changing the existing schema v2.

**Planned composition internals:** app-scoped read-only typed selectors → bounded dependency invalidation → Qt rendering; a host-only typed event/action dispatcher → SQLite transactions; normalized external source snapshots → the same view-port API; semantic preview before any persistent mutation or newly requested network permission. These are gated by opt-in schema v3, deterministic tests and the R1C tickets, and must not be presented as working functionality yet.
