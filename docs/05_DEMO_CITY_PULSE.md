# City Pulse — First Public-Data Demonstration

## Demo pitch

A complete **no-key public-data app** with an immediate result. User finds a city, sees current weather, seven-day forecast, air-quality index, and uses ChatGPT to request an **additional humidity card**. App saves this new form locally, and runs it without AI.

## External data providers

- City geocoding: https://open-meteo.com/en/docs/geocoding-api
- Weather: https://open-meteo.com/en/docs
- Air quality: https://open-meteo.com/en/docs/air-quality-api
- Terms (important): https://open-meteo.com/en/terms

No API key is required on the **free noncommercial API**. Its conditions include attribution to Open-Meteo, CC BY 4.0, noncommercial usage and rate limits. Public commercial release, paid app or promotional/commercial usage must re-evaluate these terms, procure a suitable subscription or replace the data feed. Never promise SLA for a free public API.

## Concrete HTTPS requests

Search city (escaped query):

```http
GET https://geocoding-api.open-meteo.com/v1/search?name=Paris&count=5&language=fr&format=json
```

Read `results[].name`, `country`, `latitude`, `longitude`. If zero results, show an empty state. Never assume first city is always correct: for final MVP let user choose among several matches (the starter selects first for quick bootstrap).

Current and daily weather (lat/lon from selected city):

```http
GET https://api.open-meteo.com/v1/forecast?latitude=48.8566&longitude=2.3522&current=temperature_2m,relative_humidity_2m,wind_speed_10m&daily=temperature_2m_max,temperature_2m_min,precipitation_sum&timezone=auto&forecast_days=7
```

Key paths: `current.temperature_2m`, `current.relative_humidity_2m`, `current.wind_speed_10m`; arrays `daily.time`, `daily.temperature_2m_max`, `daily.temperature_2m_min`, `daily.precipitation_sum`. Render real units from `current_units` / `daily_units` or normalize explicitly.

Air quality:

```http
GET https://air-quality-api.open-meteo.com/v1/air-quality?latitude=48.8566&longitude=2.3522&current=european_aqi,pm2_5,pm10&timezone=auto
```

Key path: `current.european_aqi`. AQI is a modeled indicator, **not** a medical or safety guarantee; do not present it as personalized health guidance.

## UX composition: phone and desktop

| Component | Mobile | Desktop |
|---|---|---|
| `CitySearch` | full-width input + search | same, in toolbar/card |
| `WeatherSummary` | large temperature card | wide summary card |
| `ForecastList` | vertically stacked daily rows | table/list in left column |
| `AirQualityCard` | a compact separate card | right column |
| `HumidityCard` | added by mutation | added by mutation |
| AI composer | bottom sheet | collapsible right panel |
| History | sheet/page | popover/sidebar |

Initial user input: **“Добавь отдельную карточку влажности после прогноза.”**

Expected model output is a JSON mutation `addWidget` with `kind: humidity-card`, after the forecast widget. **AI doesn't calculate humidity**; it chooses a view over data already received from the public API. The renderer provides the widget, the data model provides the number.

## Success demonstration checklist

1. Open City Pulse, default Paris displays real readings.
2. Search Lyon or Tokyo; results refresh and AQI changes if data is available.
3. Disconnect ChatGPT / skip sign-in. Weather APIs and UI continue to work.
4. Sign in via ChatGPT and obtain consent to plan usage; send mutation request.
5. Show preview exactly describing an added widget and no new permissions.
6. Apply. New generation, widget visible.
7. Close and reopen with AI unavailable. Widget remains and updates when public API does.
8. Restore previous app generation. Widget disappears; remote data is unchanged.

## Demo fallback and network resilience

Bundle a *clearly labeled sample snapshot* for screenshots or offline demonstration, separate from real live readings. Do not silently show fixture data as current live weather. Cache previous successful public responses with timestamps and say “Last updated ...”. Timeouts and 429 should display a retry affordance instead of blocking the GUI.

## Optional extension AFTER first milestone

Add a second independent provider for geographic country facts or use local user notes SQLite. This demonstrates genuine multi-source composition, but must not block the original City Pulse AI-mutation demo.

## Saved default city

The starter now supports city-search.defaultCity via the same validated mutation/preview path. Legacy documents without it use Paris. Apply changes the shared weather startup city; restart restores it. Manual search is temporary, and preview does not request candidate-city weather. See [17_DEFAULT_CITY.md](17_DEFAULT_CITY.md).
