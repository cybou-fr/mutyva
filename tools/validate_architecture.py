#!/usr/bin/env python3
"""Architecture fitness gate: local links, decision format, ASR/risk traceability.

Does not infer that future design docs are implemented and does not replace Qt 6.12 tests.
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
docs = [ROOT / p for p in [
    "docs/23_ARCHITECTURE_ASSESSMENT.md",
    "docs/24_ARCHITECTURE_WORKFLOW.md",
    "docs/CASE_STUDY_FR.md",
    "docs/adr/README.md",
]]
adrs = sorted((ROOT / "docs/adr").glob("[0-9][0-9][0-9][0-9]-*.md"))
errors = []

def read(path):
    if not path.is_file():
        errors.append(f"Missing architecture artifact: {path.relative_to(ROOT)}")
        return ""
    return path.read_text(encoding="utf-8")

baseline = read(docs[0])
workflow = read(docs[1])
if len(adrs) < 4:
    errors.append("Four documented architectural alternatives required")
for identifier in [f"ASR-{i:02d}" for i in range(1, 11)]:
    if identifier not in baseline:
        errors.append(f"Missing baseline ASR: {identifier}")
for identifier in [f"AR-{i:02d}" for i in range(1, 10)]:
    if identifier not in baseline:
        errors.append(f"Missing risk: {identifier}")
for term in ["Definition of Ready", "Definition of Done", "IMPLEMENTED", "STATIC-PASS", "QT-PASS", "E2E-PASS"]:
    if term not in workflow:
        errors.append(f"Missing workflow evidence gate: {term}")
for adr in adrs:
    text = read(adr)
    if not re.search(r"^# ADR-\d{4}\s", text, re.M):
        errors.append(f"{adr.name}: malformed ADR heading")
    if not all(t in text for t in ["**Status:**", "## Context", "## Options evaluated"]):
        errors.append(f"{adr.name}: requires status, context and alternatives")
    if len(re.findall(r"^- \*\*[A-D]\.", text, re.M)) < 2:
        errors.append(f"{adr.name}: needs at least two alternatives")
docs.extend(adrs)
for md in docs:
    content = read(md)
    # Link resolution only; remote citations/anchor links are not file paths.
    for dest in re.findall(r"\]\(([^)]+)\)", content):
        dest = dest.split("#", 1)[0].strip()
        if not dest or dest.startswith(("https://", "http://", "mailto:", "sandbox:")):
            continue
        resolved = (md.parent / dest).resolve()
        if not resolved.is_relative_to(ROOT.resolve()) or not resolved.is_file():
            errors.append(f"{md.relative_to(ROOT)}: broken relative link {dest}")
if errors:
    raise SystemExit("Architecture fitness FAIL:\n- " + "\n- ".join(errors))
print(f"Architecture fitness OK: {len(adrs)} ADRs, 10 ASRs, 9 risks, review/workflow links and gates")
