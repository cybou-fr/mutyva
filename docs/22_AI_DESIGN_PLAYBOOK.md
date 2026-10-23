# AI Composer Playbook — machine-readable capabilities and quality gates

**Applied-now guidance plus a proposed next-stage architecture, 2026-10-23.** [Static AI contract CI passed](https://github.com/cybou-fr/mutyva/actions/runs/39955309544). [Qt 6.12 CTest remains blocked](https://github.com/cybou-fr/mutyva/actions/runs/39955309495) on official installer secrets. The AI must design a **working application using real compiled Qt 6.12 components**, not just produce an impressive JSON tree. The corresponding small machine-readable summary is in [`runtime/designer-capabilities.json`](../runtime/designer-capabilities.json) and is sent alongside schema and component catalog by `DesignerContext` after this change.

## What AI actually receives (current `contextVersion:1`)

- **Trust-bound instructions:** `runtime/designer-rules.txt` (separate string).
- **Grounded data:** current validated app document and generation, installed 21-kind component catalog, bundled app/mutation/designer-response schemas, bounded user request.
- **New composition guide:** an app-independent, trusted list of supported workflows and explicit missing capabilities, from bundled read-only JSON. Its entries are examples and feature statuses, **not executable instructions** and not authorization.
- **NOT included:** private user records, live widget draft values, connector payloads, credentials, session tokens, file contents, other local apps or inferred account history.

The context is capped at **128 KiB**, and the user request at **4000 characters**. Keep the guide concise and deterministic; do not embed a large documentation corpus or unrelated screenshots.

## A strict internal planning sequence (no user-visible complexity)

1. **Identify user outcome.** What can the user do after Apply? Determine edit existing vs create independent app. In the current runtime, only edit existing is supported; independent app creation must return `unsupported`.
2. **Inventory only real capabilities.** Resolve widget kinds/properties/action shapes against the catalog/schemas. Future `bindings`, generic events, layout columns, arbitrary REST etc. are **not** implemented; never invent them.
3. **Plan data ownership.** Configuration vs persisted widget value vs local record model vs external snapshot. Stable IDs and default field values are essential. No user records in the context.
4. **Plan user interaction.** A trigger must map to an existing **host-owned** action; no fake `onClicked`, `SQL`, `fetch()` or arbitrary scripts. `record-form` owns create/update; `record-list` owns select/delete and confirmation. A static `button` can only append or clear a generic list.
5. **Plan layout.** Use existing `parent` and `card/stack/row/tabs`, not unsupported `grid`, `width`, `breakpoints`. Keep 360dp touch targets and desktop grouping readable.
6. **Plan external access.** Only installed Open-Meteo with existing allowed `dataSources` and permissions. Editing current app cannot add/widen external providers. Missing requested external data → precise unsupported response.
7. **Choose minimum mutation.** Preserve unchanged IDs, existing rows and sources. Order `addModel` before `addWidget` in a mutation; prefer adding a field instead of replacing the model; never delete fields with user data.
8. **Predict changes and reject false claims.** Describe what works immediately, what is a static placeholder, what is unsupported, and whether UI-only change or data/permission change requires consent. Don't claim actions were performed prior to Apply.
9. **Emit only `proposal`, `clarification` or `unsupported`** exactly as `designerResponseSchema` specifies; host revalidates everything. No preliminary speculative code block in the model response.

## Guided recipes that are valid in today's capabilities

| User says | Working composition | Where data lives | Limitation |
|---|---|---|---|
| «Сделай заметки» | `input` + `button(append-item)` + `list` + optional clear button, nested in `card` | Existing widget_data inputs/lists | Not structured full-text search |
| «Менеджер задач» *inside current app* | `addModel(tasks)` + `record-form` + `record-list` with typed filter/sort | SQLite app-scoped records | Cannot create an independent second app yet |
| «Добавь срок и приоритет» | `addField` with typed defaults; optionally adjust `record-list.sort/filter` | Existing record IDs preserved | No multi-column table or computed "overdue" expression |
| «Панель погоды» | existing Open-Meteo `city-search` + `weather-card`/`forecast-list` etc. | Trusted connector snapshot | Not a generic dashboard bound to arbitrary fields |
| «Вкладки и две панели» | `tabs` children, nested `card/row/stack`; responsive row wrap | UI definition only | No custom grid fractions or hidden script callbacks |

Do not fabricate a data-binding relationship between `select` and `record-list.filter`: today select values are persisted separately but are **not** wired to update a record filter. An app may show both visually, but that is not proof they communicate.

## Common grounded anti-examples (must not be proposed today)

- `{"kind":"chart","bind":"tasks.total"}` — no registered chart or aggregate binding.
- `{"kind":"text","bind":"tasks.selected.title"}` — text has no dynamic `bind` port yet.
- `{"kind":"button","action":{"kind":"fetch","url":"https://example.com"}}` — forbidden new network side effect.
- `{"layout":{"columns":3}}`, `{"onClicked":"..."}` — not in schemaVersion 2.
- `{"dataSources":[{"provider":"custom","url":"..."}]}` — not an installed connector.

Return `unsupported` with a clear explanation and a useful **actually supported** narrower variant rather than silent degradation. For ambiguous user intent, prefer one short clarification only when a real choice blocks correct implementation.

## Proposed capability-graph upgrade (not in v2)

After the next validated contract is implemented, the AI packet can include an inferred **typed capability graph** (all taken from the host registry):

```json
{
  "node": {"kind": "record-list", "version": 1},
  "reads": [{"port": "records", "type": "list<record>"}],
  "events": [{"name": "recordSelected", "payload": {"recordId": "string"}}],
  "capabilities": ["records.read", "records.select"],
  "layout": {"container": false, "compact": true}
}
```

This is **illustrative future metadata**, NOT a new v2 component property. The graph only describes compiled host capabilities; it is not AI-authored code. Host validates `port`, `event`, `action` and source references before preview. Use target contracts in [doc20](20_COMPOSITION_CONTRACT.md) and [doc21](21_CONNECTOR_CONTRACT.md).

## Acceptance examples / quality metrics

**Functional**, not token-count or number-of-widgets success:
- A CRUD task manager has at least `title`, `done`, a form and list, actual create/update/delete, sane empty/error states, restart persistence and stable IDs.
- After asking to add priority/due date, previous IDs and values remain; preview shows migration effect and no user-record data goes to AI.
- A compact viewport shows form and list with reachable controls, reasonable reading order and no hidden side effects.
- A second compiled external source, when eventually enabled, normalizes data and its failure is presented honestly.
- For 10 fixed user-intent fixtures, all valid proposals pass current JSON/semantic validation; invalid/unavailable features return `unsupported`, and no change happens before Apply. Replay across renderer and engine versions.

## Next step after the docs land

The nearest critical path remains **R0-02/R0-03** recovery/history and meaningful preview, plus the **Qt 6.12 gate** (official installer secrets not yet configured). Then complete R1 data migration and **C1 selection→text** as the first *real* cross-component binding, followed by host-only action routing and narrow responsive layout. Only then wire bounded normalized external-source selectors and move to R2/R3 multi-app creation.
