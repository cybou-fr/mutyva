# ADR-0001 — Local declarative modular monolith

**Status:** Accepted; implementation continues · **Date:** 2026-10-23 (formalized from prior product decisions) · **Related:** ASR-02/03/08/09, R0–R4.

## Context

MUTYVA must let AI adapt an app while its **saved native application runs without AI**. The design must be local, portable, testable, and controllable by a small maintainer, with no server/accounts/marketplace owned by MUTYVA, no plugin-code execution, and **Qt 6.12.x + C++20/QML**.

## Options evaluated

- **A. Native client with trusted declarative runtime and in-process modules — selected.** Stable data/permission/renderer contracts; easiest offline story and low operational overhead. Requires discipline to prevent large multipurpose classes.
- **B. Web shell + cloud backend or microservices — rejected for MVP.** Easier remote shared state and deployments, but adds hosting, accounts, consistency, privacy and cost not justified by a single-user offline goal.
- **C. AI-generated runnable QML/C++/JS/plugins — rejected.** Maximum expressiveness at the cost of uncontrolled execution, portability, migration and threat surface.
- **D. Separate plugin/worker sandbox per widget — deferred.** Could isolate untrusted native extensions, but the product explicitly does not load such extensions and process boundaries would multiply contracts without a demonstrated need.

## Decision

A **single native Qt process** executes versioned validated JSON declarations with **precompiled component kinds**. Host-owned code implements the local record store, mutation/history, AI transport, permissions and vetted external data. Use ordinary C++ module boundaries inside the monolith and test their edges before splitting libraries/processes. AI is a **proposal author**, never the executable.

## Consequences and reversibility

**Benefits:** offline local execution; minimal infrastructure; deterministic schema validation; clear compatibility and ownership; small cost of deployment. **Costs:** runtime palette is bounded; trusted C++ module coherence must be actively maintained; no out-of-box third-party app ecosystem or multi-user sync. **Revisit** only after a measured use case needs distribution/process isolation or remote collaboration; consider a separate optional architecture rather than silently converting the whole product to server-first.

## Fitness evidence

Source: `starter/src/main.cpp`, `starter/src/DeclarativeRuntime.cpp`, `starter/qml/WidgetHost.qml`, `starter/src/RecordStore.cpp`. Static catalog checks passed in [contract CI](https://github.com/cybou-fr/mutyva/actions/runs/39955309544). The required Qt 6.12 CI was **blocked before build**, so this ADR does not imply Qt 6.12 acceptance or Android parity.
