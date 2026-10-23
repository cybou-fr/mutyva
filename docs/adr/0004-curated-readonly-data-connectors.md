# ADR-0004 — Curated read-only connectors before general external access

**Status:** Accepted direction; only Open-Meteo adapter exists · **Date:** 2026-10-23 · **Related:** ASR-02/03/07, AR-06, C4a/C4b.

## Context

A key product promise is that AI-created apps can show external live data, but model-defined arbitrary URLs, dynamic headers and credential forwarding create SSRF, exfiltration, reliability and lifecycle risks. The shipped weather component uses a fixed trusted C++ Open-Meteo adapter.

## Options evaluated

- **A. Arbitrary REST/SQL connector generated from user/AI input — rejected for MVP.** Flexible, but unbounded hosts, secret storage, pagination, mutation privileges, schema drift and privacy.
- **B. Registered read-only adapters with fixed hosts, typed snapshots and explicit permission preview — selected direction.** Harder to cover every service, but bounded, testable and understandable.
- **C. Hardcode all external data directly into every specialized widget — current narrow slice, not scalable.** Good first Open-Meteo demonstration; prevents generic views and reused state.
- **D. Hosted central connector service — rejected under no-MUTYVA-backend constraint.** Adds server/identity/deployment/hosting obligations.

## Decision / migration path

Keep network I/O inside trusted Qt C++ adapters. **C4a** normalizes existing Open-Meteo observations into typed app-scoped read-only snapshots exposed through host selectors, with explicit `loading/ready/empty/error/stale`, timestamps, source revisions and no raw payload in AI. Only after that works, **C4b** may add a second vetted public HTTPS read-only source. No arbitrary URL field, remote CRUD, model-written request DSL or reuse of ChatGPT OAuth tokens. Future authenticated connectors require another decision and threat review.

## Risks, consequences, fitness

Consequence: limited provider coverage, deliberate work per trusted adapter, strong testability and no implicit outbound data sharing. Test SSRF/redirect/host restrictions, size/time bounds, HTTP 429/500, malformed data, cancellation and stale-response race; preview/permission revocation must stop network effects. The existing WeatherService still needs byte/deadline/error hardening; **no claim that generic read-only connectors work today**. See [source contract](../21_CONNECTOR_CONTRACT.md).
