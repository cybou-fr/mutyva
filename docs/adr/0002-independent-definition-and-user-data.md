# ADR-0002 — Separate immutable definitions from mutable user data

**Status:** Accepted; core implemented, selected recovery pending · **Date:** 2026-10-23 · **Related:** ASR-01/02/04, AR-01/05, R0-02/R1.

## Context

A user may ask AI to change or restore an app interface while task rows, notes and user choices continue evolving. A GUI generation must not act as a database snapshot. Corruption and interrupted writes must not silently switch the confirmed definition.

## Options evaluated

- **A. Store all runtime state inside each JSON UI generation — rejected.** Easy all-in-one snapshots but would copy private rows into AI context/history, cause rollback-driven data loss, and scale poorly.
- **B. Separate append-only definition generations from app-scoped SQLite data — selected.** Independent lifecycle, recovery and privacy rules.
- **C. One mutable JSON document for everything — rejected.** Simpler initial implementation but weak corruption recovery, concurrency and history semantics.

## Decision

The definition uses **immutable generation JSON + metadata containing hash, parent pointer, UTC timestamp and description + atomically written HEAD**. Only a user-confirmed Apply writes a new definition generation. App-scoped SQLite stores widget values and typed records in distinct tables; CRUD happens under local transactions and **does not create an interface generation**. Rollback stages an earlier verified *definition*, not old private records. ChatGPT receives schema/metadata, not private stored rows by default.

**Compatibility:** existing v1/v2 snapshots remain addressable, and any v3 transition is opt-in, append-only and must have test fixtures. A snapshot hash is an **integrity check**, not a digital signature or proof against malicious local tampering.

## Consequences and remaining hazards

Users may see an older UI with newer records; migration must preserve stable model/field IDs and handle fields added since that UI. **Fail-closed recovery is unresolved when HEAD is absent and old generation files remain** (AR-01); do not auto-bootstrap a fresh default over historical files. Also unify current record projection defaults for lists and edit selection (AR-05). No claim of complete transactional atomicity across filesystem+SQLite without an explicit cross-store recovery protocol.

## Fitness evidence

Source: `AppDocument.cpp`, `LocalDataStore.cpp`, `RecordStore.cpp`; historical Windows Qt 6.8 parent/orphan and CRUD tests passed in [run 39933293033](https://github.com/cybou-fr/mutyva/actions/runs/39933293033). **Not yet accepted:** R0-02 selectable recovery/crash cases, R1 v3 migrations, Qt 6.12 re-test.
