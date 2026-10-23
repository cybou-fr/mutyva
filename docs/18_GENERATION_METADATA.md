# R0-01 — Confirmed generation ancestry and integrity

**Status:** implementation in source; Windows Qt build/CTest acceptance still required. This is the on-disk contract for new MUTYVA generations, introduced after the file-based v2 history.

## Files and publication protocol

New commits use the existing `generations/` directory:

```text
head.json                            # generation, metadataVersion=1, sha256
generation-00001.json                # immutable app definition
generation-00001.meta.json           # committed generation metadata
generation-00002.json
generation-00002.meta.json
...
```

Each `.meta.json` records `formatVersion: 1`, `generation`, `parentGeneration` (0 for a root), `parentSha256` (empty for a root), `timestampUtc` in ISO-8601 UTC, `description` (bounded short first-line summary), and `sha256` of its own app definition. The digest is SHA-256 of the Qt `QJsonDocument::Compact` serialization of the parsed JSON object, not of the pretty-printed disk bytes. The hash is an integrity check, **not** an authenticity signature or protection against malicious local-file rewrites.

Under the existing `QLockFile` writer lock, publication is always: (1) validated snapshot through `QSaveFile`, (2) metadata through `QSaveFile`, (3) HEAD through `QSaveFile`. HEAD is the commit marker. A failure before HEAD leaves uncommitted files; it must never make them parents, overwrite old files, or change the running document. The allocator skips filenames of orphan **snapshots and metadata**, so it can produce gaps in generation numbers.

At app start the HEAD-selected snapshot is validated, and a modern HEAD **requires** matching metadata and a matching HEAD digest. Corrupt/missing metadata or snapshots block subsequent saves rather than overwriting history. This is not yet a general automatic recovery procedure (R0-02).

## Confirmed parent semantics

A new committed generation always records the **previous confirmed HEAD** as its parent, and the hash of that parent's document. Rollback uses that exact parent pointer and verifies the parent hash before showing a candidate. It **never** computes `generation - 1`. Applying the preview creates a **new** generation whose parent is the formerly active HEAD. User SQLite records remain outside interface history.

Old `head.json` files with only `{"generation": N}` and old snapshots are read unchanged. Their historical chain cannot be proven without parent metadata; therefore rollback on a legacy HEAD with no recorded parent fails closed. **The next successful new-format commit** anchors that legacy HEAD as a confirmed parent and allows rollback to it, with a stored parent digest. Existing v1→v2 append migration still creates a new generation and preserves the v1 file.

## Regression matrix

`tests/RuntimeTest.cpp` adds:
- `confirmedParentSkipsOrphanAndSurvivesRestart`: valid but uncommitted generation 2, confirmed HEAD 3, rollback to 1 then commit 4; orphan remains untouched; restart reloads correct document.
- `legacyHistoryAndVerifiedParentIntegrity`: old HEAD accepted; next commit anchors legacy parent, and a valid but modified parent fails checksum verification.
- `generationMetadataCorruptionBlocksWrites`: corrupted committed metadata blocks writes and keeps previous HEAD/snapshot intact.

Run locally on configured Windows Qt kit:

```powershell
cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/msvc2022_64 -DBUILD_TESTING=ON
cmake --build build-msvc --config Release
ctest --test-dir build-msvc -C Release --output-on-failure
```

Keep R0-01 unchecked until these tests and the existing regressions pass on Qt. R0-02 (history browser/recovery/selection and further crash testing) and R0-03 (normal-user semantic diff) remain separate tickets.
