# Declarative Runtime, Components and Mutations

## Goal

Treat every MUTYVA app as a **versioned declarative document** interpreted by the installed Qt runtime. AI and manual editing both produce the same operations. No compiler, process reboot or App Store publishing per generated composition.

## Vocabulary

- **Component kind:** trusted renderer class, e.g. `weather-card`, `forecast-list`, `humidity-card`.
- **Widget instance:** stable `id`, known `kind`, typed properties and allowed data bindings.
- **Application document:** roots, widgets, sources, permission declarations and runtime-schema version.
- **Mutation:** ordered, typed changes to an application document, not general program code.
- **Generation:** a committed immutable app snapshot with its content hash, time, parent generation and provenance.

## MVP schema

See `specs/app.schema.json` and `specs/mutation.schema.json` (JSON Schema 2020-12). Current schemaVersion is 2: app id/name, ordered widgets, parent references to card/stack/row containers, per-kind properties, sources and permissions. Legacy v1 schemas are retained for validation and append-only migration. Source types are explicit (`rest`, `sqlite` later).

### Safe mutation operations (MVP)

1. `addWidget` with `widget` and optional `after` stable ID.
2. `removeWidget` by stable ID (with confirmation when state would be discarded).
3. `updateProperty` for *approved typed writable properties*, by widget ID, path and value.
4. `moveWidget` with `after`, preserving widget instance state when possible.

No free-form Qt objects, arbitrary expressions, system commands, shell, filesystem writes, SQL snippets or executable URLs in the declarative grammar.

### Implemented local editor

`DeclarativeRuntime` loads the bundled app and mutation schemas, checks their supported vocabulary and enforces unique widget/source IDs, registered component kinds, required fixed sources and the Open-Meteo permission allowlist. It constructs a candidate copy and only returns it after the entire ordered batch validates. Application declarations loaded from disk also pass this validation.

All four operations are available from **Изменить интерфейс**, and JSON can be pasted/edited as a declaration or mutation. updateProperty is restricted to the selected kind's catalogued properties. An omitted addWidget.after appends within its parent; an empty after inserts/moves to the beginning. Nonempty anchors must be siblings in the same container. moveWidget may set parent; cycles and moving after oneself are rejected. Removing a container removes its subtree, with the full diff shown in preview. See `12_GENERAL_PALETTE.md` for actions and session data.

Preview renders the candidate without changing the active document or disk. Apply saves a new generation; Cancel restores the committed view. Whole-document editing cannot change the application ID, data sources or permissions in this slice. Deletion also drops the removed generic controls' session values, with a preview warning; it does not delete external data. Delegates are recreated, but generic input/list state is kept separately by stable ID. The weather search field's transient view state is not reconciled yet.

## Pipeline / invariants

```
User request -> AI JSON -> Parse -> JSON Schema validation
  -> Semantic validation (kind and props exist; references resolve)
  -> Permission / side-effect analysis -> Candidate document
  -> Preview + user approval -> Atomic persist and switch HEAD
  -> Render with stable-ID reconciliation
```

**Invariant A:** a failed mutation does not change the running app or HEAD.  
**Invariant B:** a valid mutation must be replayable with no AI dependency.  
**Invariant C:** history is append-only; restore produces a new generation and retains prior snapshots.  
**Invariant D:** identical locked component definitions yield equivalent functionality across supported platforms; visual measurements may differ.  
**Invariant E:** external API/data actions are outside config rollback.

## Generation storage layout

```
app-data/
  apps/city-pulse/
    head.json
    generations/000001.json
    generations/000002.json
    generations/000003.json
  cache/open-meteo/
  mutyva.sqlite
```

Full store should use an SQLite transaction that writes a new snapshot, metadata, parent link, hash and HEAD consistently. If assets are external files, write content blobs before final metadata commit, use hash-based paths, and garbage-collect unreachable blobs later. Basic starter already demonstrates append-only generation files and an atomically replaced HEAD, but lacks full schema validation and DB-backed history browser.

## Registry plan

A local, static core palette compiled into Qt QML resources: `Text`, `Button`, `Stack`, `Row`, `Card`, `List`, `DataTable` (later), `CitySearch`, `WeatherSummary`, `ForecastList`, `AirQualityCard`, `HumidityCard`. An optional local `components/` folder can contain **declarative compound components**, not arbitrary QML code downloaded from anyone.

Each kind has descriptor:

```json
{
  "kind": "humidity-card",
  "version": 1,
  "properties": {"title": {"type":"string"}},
  "requiredBindings": ["weather.current.humidity"],
  "permissions": []
}
```

Only the preinstalled Qt QML implementation renders the above kind. Later custom components are compositions of pre-existing safe kinds.

## Declarative bindings

Avoid string evaluation. A reference such as `weather.current.humidity` must be resolved by typed lookup in the host's in-memory data model, not by injecting expression strings into QML. For MVP, a fixed per-widget contract is sufficient; introduce a generic binding engine after the AI mutation path works.

## AI can propose but not commit

For example: `examples/mutations/add-humidity-card.json`. The application lets a human review the new component, placement, dependencies and any requested permissions. The engine validates and commits only on explicit Apply. A button click does not implicitly grant a new network domain.

## Full Visual RAD designer is later

Initial preview + basic reorder/switch is enough. Pixel-perfect designer, drag-drop toolbox, connection graph, dynamic end-user expression language and component code editor are **not** MVP gates.
