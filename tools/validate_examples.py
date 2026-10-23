#!/usr/bin/env python3
"""Small dependency-free integrity checks; if jsonschema is installed, validate full Draft 2020-12 examples."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def load(path):
    return json.loads((ROOT / path).read_text(encoding="utf-8"))


app_schema = load("specs/app.schema.json")
mutation_schema = load("specs/mutation.schema.json")
response_schema = load("specs/designer-response.schema.json")
for schema in (app_schema, mutation_schema, response_schema):
    assert schema["$schema"].endswith("2020-12/schema"), "Unsupported schema dialect"

apps = [
    load("examples/city-pulse.mutyva.json"),
    load("examples/general-workspace.mutyva.json"),
    load("examples/task-manager.mutyva.json"),
]
responses = [
    load("examples/designer/add-humidity-response.json"),
    load("examples/designer/default-city-poitiers-response.json"),
]
known_kinds = set(app_schema["$defs"]["widget"]["properties"]["kind"]["enum"])
known_ops = {"addWidget", "removeWidget", "updateProperty", "moveWidget", "addModel", "addField"}

for app in apps:
    assert app["schemaVersion"] == 2, "Only current v2 executable examples are supported"
    ids = [w["id"] for w in app["widgets"]]
    assert len(ids) == len(set(ids)), "Duplicate widget IDs"
    assert all(w["kind"] in known_kinds for w in app["widgets"]), "Unregistered component"
    assert all(s["provider"] == "open-meteo" for s in app["dataSources"]), "Unsupported example connector"

for response in responses:
    assert response["status"] == "proposal", "Unexpected example response type"
    mutation = response["mutation"]
    assert mutation["schemaVersion"] == 2, "Mutation version mismatch"
    assert mutation["appId"] == apps[0]["id"], "Mutation references wrong app"
    assert all(op["op"] in known_ops for op in mutation["operations"]), "Unknown mutation operation"

print("MUTYVA examples: structure and allowlisted providers/kinds/actions OK")

try:
    import jsonschema
    from referencing import Registry, Resource
except ImportError:
    print("Optional full checks: pip install jsonschema referencing")
else:
    for schema in (app_schema, mutation_schema, response_schema):
        jsonschema.Draft202012Validator.check_schema(schema)
    registry = Registry()
    for schema in (app_schema, mutation_schema, response_schema):
        registry = registry.with_resource(schema["$id"], Resource.from_contents(schema))
    for app in apps:
        jsonschema.Draft202012Validator(app_schema, registry=registry).validate(app)
    for response in responses:
        jsonschema.Draft202012Validator(response_schema, registry=registry).validate(response)
    print("MUTYVA examples: full Draft 2020-12 validation OK")
