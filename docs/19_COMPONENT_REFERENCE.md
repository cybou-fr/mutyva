# MUTYVA — Component Reference (contract v2.1, 2026-10-23)

This is the **actual allowlisted palette shipped by the current source**, not a list of Qt Quick capabilities. It is generated from `runtime/component-catalog.json` and `specs/app.schema.json` and checked by `tools/validate_palette.py`.

**21 kinds = 5 Open-Meteo widgets + 16 general/data UI widgets.** New properties and models are *backwards-compatible optional additions to schemaVersion 2*, deliberately avoiding an untested v3 migration. The v3 breaking-version gate is **not** complete. Legacy documents without `models` still validate.

## Global rules

- Every widget has a stable `id`, a registered `kind` and optionally `title` and `parent`. The `parent` must refer to a `card`, `stack`, `row` or `tabs`. Max 100 widgets and 8 levels. Unknown properties are rejected per kind.
- `style` is an optional object `{"tone":"neutral|accent|danger","padding":0..32}`. Rendering support is basic, mostly Text/Card/Button/Icon; it is **not** a general Qt style sheet.
- The `input`, `checkbox`, `select` and `date-picker` kinds store simple values in the existing local SQLite `widget_data` table; individual widget values do not create interface generations. `record-list`/`record-form` work with host-owned `record_data` using app/model/record IDs and a separate revision; neither table contains OAuth credentials.
- `image` only permits the bundled brand resources; arbitrary external URLs, files and uploaded media are not supported. `icon` is a small trusted symbol allowlist, not a downloadable icon pack. `dialog` is informational. `tabs` displays one immediate child per tab.
- The trusted host owns CRUD, data validation, filter and sort. No AI-provided SQL, shell, JavaScript or QML is evaluated. AI can propose `addModel`/`addField`, then widgets, and users must confirm. Existing fields are immutable; new fields are additive.
- A model has `id`, `title` and typed `fields` with `id`, `label`, `type` (`text`, `integer`, `boolean`, `date`), optional `required`, optional typed `default`. Required fields must provide defaults for safe migrations. `record-list.filter` uses `{"field":"done","op":"equals","value":false}`; `sort` uses `{"field":"priority","descending":true}`. `before`/`after` are limited to date/integer.
- Limits: up to 12 models and 24 fields each; 1000 local records / 4 MiB per app; at most 16 KiB JSON per record. A record ID is host generated. Deletion of a record requires confirmation from the trusted QML UI. Interface rollback never rolls back user records.

## Catalog reference

Every entry below includes its concrete declaration example. Screenshots are **not bundled** because a verified Qt renderer is required; `MUTYVA_PALETTE_CAPTURE_DIR` enables the Qt test harness to capture each kind separately on a configured Qt build. Do not represent renders as verified until files are produced by that harness.

### `city-search` — Поиск города

Поиск города через Open-Meteo. defaultCity задаёт общий город при запуске и после применения изменения; без свойства используется Paris. Несколько поисков разделяют общий город, заданные defaultCity должны совпадать.

**Writable properties:** `title`, `defaultCity`. **Actions:** `searchCity`. **Bindings:** `city.search`. **Permissions:** `network:*.open-meteo.com`.

```json
{
  "id": "exampleWidget",
  "kind": "city-search",
  "title": "Поиск города",
  "defaultCity": "Paris"
}
```

**Example AI request:** «Добавь компонент поиск города с ID exampleWidget, используя только доступные свойства каталога; если нужны зависимости, объясни их».

### `weather-card` — Текущая погода

Температура и скорость ветра для выбранного города.

**Writable properties:** `title`. **Actions:** none. **Bindings:** `weather.current.temperature`, `weather.current.wind`. **Permissions:** `network:*.open-meteo.com`.

```json
{
  "id": "exampleWidget",
  "kind": "weather-card",
  "title": "Текущая погода"
}
```

**Example AI request:** «Добавь компонент текущая погода с ID exampleWidget, используя только доступные свойства каталога; если нужны зависимости, объясни их».

### `forecast-list` — Прогноз на 7 дней

Список дат, минимальной и максимальной температуры и осадков.

**Writable properties:** `title`. **Actions:** none. **Bindings:** `weather.daily`. **Permissions:** `network:*.open-meteo.com`.

```json
{
  "id": "exampleWidget",
  "kind": "forecast-list",
  "title": "Прогноз на 7 дней"
}
```

**Example AI request:** «Добавь компонент прогноз на 7 дней с ID exampleWidget, используя только доступные свойства каталога; если нужны зависимости, объясни их».

### `air-card` — Качество воздуха

European Air Quality Index выбранного города.

**Writable properties:** `title`. **Actions:** none. **Bindings:** `air.current.european_aqi`. **Permissions:** `network:*.open-meteo.com`.

```json
{
  "id": "exampleWidget",
  "kind": "air-card",
  "title": "Качество воздуха"
}
```

**Example AI request:** «Добавь компонент качество воздуха с ID exampleWidget, используя только доступные свойства каталога; если нужны зависимости, объясни их».

### `humidity-card` — Влажность воздуха

Относительная влажность из уже загружаемого прогноза. Отдельный API-запрос не нужен.

**Writable properties:** `title`. **Actions:** none. **Bindings:** `weather.current.humidity`. **Permissions:** `network:*.open-meteo.com`.

```json
{
  "id": "exampleWidget",
  "kind": "humidity-card",
  "title": "Влажность воздуха"
}
```

**Example AI request:** «Добавь компонент влажность воздуха с ID exampleWidget, используя только доступные свойства каталога; если нужны зависимости, объясни их».

### `text` — Текст

Обычный текст без HTML и выражений.

**Writable properties:** `style`, `title`, `text`. **Actions:** none. **Bindings:** none. **Permissions:** none.

```json
{
  "id": "exampleWidget",
  "kind": "text",
  "title": "Текст",
  "text": "Новый текст"
}
```

**Example AI request:** «Добавь компонент текст с ID exampleWidget, используя только доступные свойства каталога; если нужны зависимости, объясни их».

### `card` — Карточка

Контейнер с фоном и заголовком. Вложенные виджеты ссылаются на его ID через parent.

**Writable properties:** `style`, `title`. **Actions:** none. **Bindings:** none. **Permissions:** none.

```json
{
  "id": "exampleWidget",
  "kind": "card",
  "title": "Карточка"
}
```

**Example AI request:** «Добавь компонент карточка с ID exampleWidget, используя только доступные свойства каталога; если нужны зависимости, объясни их».

### `stack` — Колонка

Вертикальный контейнер вложенных компонентов.

**Writable properties:** `style`, `title`. **Actions:** none. **Bindings:** none. **Permissions:** none.

```json
{
  "id": "exampleWidget",
  "kind": "stack",
  "title": "Колонка"
}
```

**Example AI request:** «Добавь компонент колонка с ID exampleWidget, используя только доступные свойства каталога; если нужны зависимости, объясни их».

### `row` — Строка

Горизонтальный контейнер; на узком экране элементы располагаются вертикально.

**Writable properties:** `style`, `title`. **Actions:** none. **Bindings:** none. **Permissions:** none.

```json
{
  "id": "exampleWidget",
  "kind": "row",
  "title": "Строка"
}
```

**Example AI request:** «Добавь компонент строка с ID exampleWidget, используя только доступные свойства каталога; если нужны зависимости, объясни их».

### `button` — Кнопка

Пользователь запускает разрешённое действие append-item или clear-list. Без action кнопка неактивна.

**Writable properties:** `style`, `title`, `action`. **Actions:** `append-item`, `clear-list`. **Bindings:** none. **Permissions:** none.

```json
{
  "id": "exampleWidget",
  "kind": "button",
  "title": "Кнопка",
  "action": {
    "kind": "clear-list",
    "target": "notesList"
  }
}
```

**Example AI request:** «Добавь компонент кнопка с ID exampleWidget, используя только доступные свойства каталога; если нужны зависимости, объясни их».

### `input` — Поле ввода

Локальное текстовое значение в SQLite, до 500 символов.

**Writable properties:** `title`, `value`, `placeholder`. **Actions:** none. **Bindings:** none. **Permissions:** none.

```json
{
  "id": "exampleWidget",
  "kind": "input",
  "title": "Поле ввода",
  "value": "",
  "placeholder": "Введите текст"
}
```

**Example AI request:** «Добавь компонент поле ввода с ID exampleWidget, используя только доступные свойства каталога; если нужны зависимости, объясни их».

### `list` — Список

Локальный список строк в SQLite, до 50 элементов.

**Writable properties:** `title`, `items`. **Actions:** none. **Bindings:** none. **Permissions:** none.

```json
{
  "id": "exampleWidget",
  "kind": "list",
  "title": "Список",
  "items": []
}
```

**Example AI request:** «Добавь компонент список с ID exampleWidget, используя только доступные свойства каталога; если нужны зависимости, объясни их».

### `record-form` — Форма записей

Создаёт и редактирует типизированные записи локальной модели. Данные хранятся отдельно от истории интерфейса.

**Writable properties:** `title`, `model`, `style`. **Actions:** `create-record`, `update-record`. **Bindings:** none. **Permissions:** none.

```json
{
  "id": "exampleWidget",
  "kind": "record-form",
  "title": "Форма записей",
  "model": "tasks"
}
```

**Example AI request:** «Добавь компонент форма записей с ID exampleWidget, используя только доступные свойства каталога; если нужны зависимости, объясни их».

### `record-list` — Список записей

Просмотр, выбор и удаление записей модели; необязательные типизированные filter и sort.

**Writable properties:** `title`, `model`, `filter`, `sort`, `style`. **Actions:** `select-record`, `delete-record`. **Bindings:** none. **Permissions:** none.

```json
{
  "id": "exampleWidget",
  "kind": "record-list",
  "title": "Список записей",
  "model": "tasks",
  "filter": {
    "field": "done",
    "op": "equals",
    "value": false
  },
  "sort": {
    "field": "priority",
    "descending": true
  }
}
```

**Example AI request:** «Добавь компонент список записей с ID exampleWidget, используя только доступные свойства каталога; если нужны зависимости, объясни их».

### `checkbox` — Флажок

Логическое локальное значение true/false, хранится в SQLite.

**Writable properties:** `title`, `value`, `style`. **Actions:** none. **Bindings:** none. **Permissions:** none.

```json
{
  "id": "exampleWidget",
  "kind": "checkbox",
  "title": "Флажок",
  "value": "false"
}
```

**Example AI request:** «Добавь компонент флажок с ID exampleWidget, используя только доступные свойства каталога; если нужны зависимости, объясни их».

### `select` — Выбор

Выбор одной строки из до 20 предопределённых вариантов.

**Writable properties:** `title`, `value`, `options`, `style`. **Actions:** none. **Bindings:** none. **Permissions:** none.

```json
{
  "id": "exampleWidget",
  "kind": "select",
  "title": "Выбор",
  "options": [
    "Первый",
    "Второй"
  ],
  "value": "Первый"
}
```

**Example AI request:** «Добавь компонент выбор с ID exampleWidget, используя только доступные свойства каталога; если нужны зависимости, объясни их».

### `date-picker` — Дата

Локальное ISO-дата YYYY-MM-DD, редактируется с проверкой.

**Writable properties:** `title`, `value`, `style`. **Actions:** none. **Bindings:** none. **Permissions:** none.

```json
{
  "id": "exampleWidget",
  "kind": "date-picker",
  "title": "Дата",
  "value": ""
}
```

**Example AI request:** «Добавь компонент дата с ID exampleWidget, используя только доступные свойства каталога; если нужны зависимости, объясни их».

### `dialog` — Диалог

Кнопка открывает доверенный Qt Quick Dialog с текстом; без произвольных обработчиков.

**Writable properties:** `title`, `message`, `style`. **Actions:** `open-dialog`. **Bindings:** none. **Permissions:** none.

```json
{
  "id": "exampleWidget",
  "kind": "dialog",
  "title": "Диалог",
  "message": "Информация"
}
```

**Example AI request:** «Добавь компонент диалог с ID exampleWidget, используя только доступные свойства каталога; если нужны зависимости, объясни их».

### `tabs` — Вкладки

Контейнер: каждый прямой ребёнок образует отдельную вкладку.

**Writable properties:** `title`, `style`. **Actions:** none. **Bindings:** none. **Permissions:** none.

```json
{
  "id": "exampleWidget",
  "kind": "tabs",
  "title": "Мои вкладки"
}
```

**Example AI request:** «Добавь компонент вкладки с ID exampleWidget, используя только доступные свойства каталога; если нужны зависимости, объясни их».

### `image` — Изображение

Только встроенные ресурсы бренда. Произвольные URL и файловые пути запрещены.

**Writable properties:** `title`, `source`, `style`. **Actions:** none. **Bindings:** none. **Permissions:** none.

```json
{
  "id": "exampleWidget",
  "kind": "image",
  "title": "Изображение",
  "source": "mutyva-symbol"
}
```

**Example AI request:** «Добавь компонент изображение с ID exampleWidget, используя только доступные свойства каталога; если нужны зависимости, объясни их».

### `icon` — Иконка

Безопасный набор символов, не требует сторонних загрузок.

**Writable properties:** `title`, `icon`, `style`. **Actions:** none. **Bindings:** none. **Permissions:** none.

```json
{
  "id": "exampleWidget",
  "kind": "icon",
  "title": "Иконка",
  "icon": "star"
}
```

**Example AI request:** «Добавь компонент иконка с ID exampleWidget, используя только доступные свойства каталога; если нужны зависимости, объясни их».

## Working composition and tests

The [Task Manager example](../examples/task-manager.mutyva.json) combines typed records, `record-form`, `record-list`, an equality filter, numeric sort, tabs, icons, image, dialog, checkbox, select and date control. It is a **local declaration for the current official.city-pulse app ID**, not yet multi-app creation. Load it using developer JSON preview; do not replace unrelated saved applications without explicit consent.

Test contracts: `bundledSchemasAndCatalog`, `typedRecordCrudAndMigration`, `paletteRecordQtRendering`, `paletteCaptureScreenshots`, plus `tools/validate_palette.py`. The screenshot harness is opt-in and requires a real functioning Qt build; screenshot paths are `<capture-dir>/<kind>.png`.

**Known limitations:** text field only (no rich text/markdown); no general network bindings; model CRUD is local; calendar supports ISO dates; style API is intentionally tiny; widgets do not have independent arbitrary callbacks; `select` is a static list rather than a DB relationship; incomplete-record filter is fixed in declaration; storage is not encrypted; v3 migration/Android verification remain follow-up gates.
