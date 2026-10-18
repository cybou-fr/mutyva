# MVP Product Requirements

## Goal

Prove six things in ONE demo: (1) live public data, (2) Qt responsive rendering, (3) a declarative component registry, (4) real ChatGPT Plus-capable sign-in, (5) one AI-proposed mutation that modifies the UI safely, (6) persistence and reopening without AI.

## Audience / scenario

A person without programming skills needs a more useful view of public data and says what to change. First built-in app: **City Pulse**, which combines city lookup, current weather, seven-day weather forecasts and air quality from Open-Meteo.

## Required screens

| Screen | Requirements |
|---|---|
| Landing / app | MUTYVA header; city selection; City Pulse with live data; status/error and refresh |
| AI Design sheet | “Continue with ChatGPT” when disconnected, natural language field when connected, visible progress/cancel |
| Mutation preview | Human-readable changes; added/removed widgets; permissions; Apply / Cancel; errors |
| Generations | Chronological successful generations; view summary and restore chosen generation as a new commit |
| Settings | Sign in/out, connection state, source attribution, local file locations and optional clear cached data |

## Key behaviors / acceptance

- The app launches and shows a usable window without ChatGPT sign-in.
- City lookup calls live HTTPS and handles no results, timeout, empty arrays and malformed JSON.
- UI rendering uses `component.type` / `kind` from JSON; no generated QML source is evaluated.
- The palette initially includes `CitySearch`, `WeatherSummary`, `ForecastList`, `AirQualityCard`, `HumidityCard`, `Text`, `Container`, `Button` or equivalents; widgets have stable IDs.
- AI sign-in uses supported OAuth + consent, persisted `client_id` and host identifier, PKCE, ID-token verification, refresh and user-controlled sign-out.
- AI request includes current document, **allowed component catalog**, app goal and a concise summary of available public data. No secrets.
- AI answer is interpreted as an explicit structured mutation or parsed JSON checked against a schema. Never execute model-produced C++/QML/JS/SQL.
- Mutations are validated (schema, kinds, stable IDs, data bindings, capabilities), previewed, then committed atomically.
- Every committed mutation produces a new immutable generation; failed/cancelled mutations do not.
- Restart restores the latest generation, including its component configuration, with no inference call.
- User can restore a prior version of the *app definition*; current public data may differ and external actions are not rolled back.
- Narrow-window/mobile layout remains usable without horizontal scrolling for ordinary content.

## User stories

**US-01** Start and query public city data. **US-02** Change city by text search. **US-03** See forecast and air quality. **US-04** Authorize ChatGPT Plus from MUTYVA. **US-05** Ask AI to add a humidity card. **US-06** Accept/reject preview. **US-07** Reopen saved mutation with AI disconnected. **US-08** Restore earlier generation. **US-09** Access used data sources and privacy notice.

## Non-functional budget (target, not a claim)

- Cold start to visible shell: target below 2 s on development desktop, excluding network.
- Local UI changes: target frame times below 16.7 ms in normal demo interaction.
- Data refresh should not freeze the UI thread; show status while awaiting APIs.
- Mutation **apply** after AI output: target below 250 ms for this small example; AI response latency is separately measured.
- No token, secret or unredacted AI prompt should appear in logs.
- Accessibility: keyboard focus, readable contrast, scalable typography and touch targets.

## Exits / scope gates

**Gate A (real data):** Windows Qt app runs with live public APIs and no AI.  
**Gate B (declarative):** renderer + local manifest + mutation history.  
**Gate C (real auth):** SIWC verified end-to-end with a consenting eligible test account.  
**Gate D (real AI mutation):** add humidity card → preview → save → restart without AI.  
**Gate E (portable):** Android proof on physical device and platform smoke strategy for macOS/Linux/iOS.

MVP is *not* declared complete after Gate A. The starter currently demonstrates a subset of A/B; see README.
