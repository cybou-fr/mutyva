# MUTYVA — Architecture Decision Records

This index collects the **actual architectural choices and their limits**, not a retrospective claim that every planned component is already implemented. ADRs explain *why* and *when to reconsider* choices; [assessment](../23_ARCHITECTURE_ASSESSMENT.md) is the current source-grounded state and [workflow](../24_ARCHITECTURE_WORKFLOW.md) is the delivery discipline.

| ADR | Status | Primary trade-off |
|---|---|---|
| [0001 — Local declarative modular monolith](0001-local-declarative-modular-monolith.md) | **Accepted / partially realized** | Lower operational complexity versus remote collaboration and unconstrained plugins |
| [0002 — Separate definition from mutable user data](0002-independent-definition-and-user-data.md) | **Accepted / implemented base, recovery incomplete** | Non-destructive UI rollback versus two distinct persistence lifecycles |
| [0003 — Host-owned typed composition](0003-host-owned-typed-composition.md) | **Proposed target (C1/C2)** | Safe typed graphs versus flexibility of arbitrary expressions |
| [0004 — Curated read-only data connectors](0004-curated-readonly-data-connectors.md) | **Accepted direction, expansion not implemented** | Trustworthy bounded adapters versus arbitrary URLs and remote writes |

**Template:** Status/date; context and requirements; options and rejected alternatives; decision; consequences/risks; tests and reconsideration trigger. Decisions are deliberately short. If a constraint changes, write a superseding ADR and update the [ASR/risk register](../23_ARCHITECTURE_ASSESSMENT.md); never quietly rewrite original history.
