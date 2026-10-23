# ADR-0003 — Host-owned typed composition instead of arbitrary expressions

**Status:** Proposed / not yet implemented as a general engine · **Date:** 2026-10-23 · **Related:** ASR-03/04/05/06/09, C1/C2/C3.

## Context

Current trusted components support fixed notes actions, model-linked record forms/lists, fixed Open-Meteo bindings and basic container layout. To create genuinely useful apps, AI must link a selected record to a text/detail view and trigger bounded record commands. It must **not** gain the ability to execute JavaScript, QML, SQL or unlimited workflow scripts.

## Options evaluated

- **A. Bind with arbitrary QML/JS expressions and unrestricted callbacks — rejected.** Rich, but opens an unreviewable code/runtime capability surface and poor migration semantics.
- **B. Tiny host-owned typed ports/selectors and one event→one validated command — preferred.** Deterministic types, bounded graph and permission checking; less expressive, but enough for Task Manager.
- **C. General reactive rules engine/workflow language — rejected for MVP.** Complicates cycles, execution budgets, security and persistence before the first simple cross-component use case works.
- **D. Hardwire every new relation inside each QML widget — interim state only.** Works for demo but scales poorly and prevents composable AI actions.

## Proposed decision

First implement **C1**: one read-only record-selection selector to a trusted text port, with stable widget/model IDs, typed null/missing/error states and app-scoped subscriptions. Then implement **C2**: a typed component event maps to one allowlisted host action with validated inputs, permission gate, transaction result and bounded success/error feedback. Extend known containers with only small responsive tokens (C3). The actual schema remains **v2** until an accepted opt-in v3 contract and migration exists. **No future field is executable merely because it is shown in a design document.**

## Consequences and proof

Benefits: reusable checked dataflow, preserved Qt-native portability, understandable AI context and preview. Costs: a deliberately limited grammar; every new port/action requires compiled host support and tests. **Required proof:** missing/wrong references and types fail; selection edit/delete updates view; no cycles or stale subscriptions; 360dp and desktop display; preview has no side effects; Qt 6.12 build/CTest. **Decision not yet green**; see [composition specification](../20_COMPOSITION_CONTRACT.md).
