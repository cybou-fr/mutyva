#!/usr/bin/env python3
"""Verify static palette/schema/QML/docs consistency without Qt or third-party modules."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
load = lambda path: json.loads((ROOT / path).read_text(encoding="utf-8"))
catalog = load("runtime/component-catalog.json")["components"]
schema = load("specs/app.schema.json")
widgets = schema["$defs"]["widget"]["properties"]
kinds = widgets["kind"]["enum"]
reference = (ROOT / "docs/19_COMPONENT_REFERENCE.md").read_text(encoding="utf-8")
host = (ROOT / "starter/qml/WidgetHost.qml").read_text(encoding="utf-8")
generic = (ROOT / "starter/qml/GeneralWidget.qml").read_text(encoding="utf-8")
records = (ROOT / "starter/qml/RecordWidget.qml").read_text(encoding="utf-8")
errors = []
names = [entry["kind"] for entry in catalog]
if len(names) != len(set(names)) or set(names) != set(kinds):
    errors.append("Catalog kinds are missing/duplicated or differ from schema")
if len(names) != 21:
    errors.append("Expected 21 catalog kinds; update the gate for intentional additions")
for entry in catalog:
    kind = entry["kind"]
    if not entry.get("description") or not entry.get("title"):
        errors.append(f"{kind}: missing user-facing description/title")
    if f"### `{kind}`" not in reference:
        errors.append(f"{kind}: missing documentation")
    if f'case "{kind}"' not in host:
        errors.append(f"{kind}: not routed by WidgetHost")
    if kind.startswith("record-"):
        if f'"{kind}"' not in records:
            errors.append(f"{kind}: missing RecordWidget rendering")
    elif kind not in ("city-search", "weather-card", "forecast-list", "air-card", "humidity-card"):
        if f'case "{kind}"' not in generic and kind not in ("card","stack","row"):
            errors.append(f"{kind}: missing GeneralWidget rendering")
    for prop in entry.get("propertyNames", []):
        if prop not in widgets:
            errors.append(f"{kind}.{prop}: not in JSON Schema")
    for binding in entry.get("bindings", []):
        if not binding.get("path"):
            errors.append(f"{kind}: missing binding path")
if errors:
    raise SystemExit("Palette contract FAILED:\n- " + "\n- ".join(errors))
print(f"Palette contract OK: {len(names)} kinds, schema, renderer routes and reference")
