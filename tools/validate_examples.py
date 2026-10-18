#!/usr/bin/env python3
"""No-dependency consistency checks, not a replacement for a JSON Schema 2020-12 validator."""
from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]
application = json.loads((root / "examples/city-pulse.mutyva.json").read_text(encoding="utf-8"))
mutation = json.loads((root / "examples/mutations/add-humidity-card.json").read_text(encoding="utf-8"))
for name in ("app.schema.json", "mutation.schema.json"):
    schema = json.loads((root / "specs" / name).read_text(encoding="utf-8"))
    assert schema["$schema"].endswith("2020-12/schema")
assert application["schemaVersion"] == mutation["schemaVersion"] == 2
assert application["id"] == mutation["appId"]
known_kinds = set(json.loads((root / "specs/app.schema.json").read_text(encoding="utf-8"))["$defs"]["widget"]["properties"]["kind"]["enum"])
widgets = application["widgets"]
ids = [w["id"] for w in widgets]
assert len(ids) == len(set(ids)), "Duplicate widget ID"
assert all(w["kind"] in known_kinds for w in widgets)
for op in mutation["operations"]:
    assert op["op"] in ("addWidget", "removeWidget", "updateProperty", "moveWidget")
    if op["op"] == "addWidget":
        assert op["widget"]["id"] not in ids
        assert op["widget"]["kind"] in known_kinds
        if "after" in op: assert op["after"] in ids
print("MUTYVA example JSON and structural invariants: OK")
try:
    import jsonschema
except ImportError:
    print("Optional: pip install jsonschema for full schema checks")
else:
    from referencing import Registry, Resource
    app_schema = json.loads((root / "specs/app.schema.json").read_text(encoding="utf-8"))
    mutation_schema = json.loads((root / "specs/mutation.schema.json").read_text(encoding="utf-8"))
    registry = Registry().with_resource("https://mutyva.invalid/specs/app.schema.json", Resource.from_contents(app_schema))
    # relative ref app.schema.json resolves against mutation schema's $id
    jsonschema.Draft202012Validator(app_schema).validate(application)
    jsonschema.Draft202012Validator(mutation_schema, registry=registry).validate(mutation)
    print("Full JSON Schema checks: OK")
