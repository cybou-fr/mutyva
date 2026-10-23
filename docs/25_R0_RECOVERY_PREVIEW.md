# R0-02 / R0-03 — Confirmed history restore and semantic preview

**Implementation candidate:** 2026-10-23. **Evidence:** source inspection and deterministic Qt test cases added; **required Qt 6.12 Windows CI not yet executed** because repository GitHub Actions secrets `QT_EMAIL` and `QT_PW` are missing. These R0 gates remain **implemented-unverified**, not accepted until the relevant Qt/MSVC CTest and UI tests pass.

## R0-02: startup and history safety

**Previously:** if `head.json` was missing, `AppDocument` unconditionally committed a fresh City Pulse generation. Existing `generation-*.json` (including orphan `.meta.json`) could thus be misunderstood as a new install.

**Now:** bootstrap is allowed **only when HEAD is absent and there are no generation files and no existing user-data database**. If any historical file or `data/user-data.sqlite` remains, the document enters a fail-closed, read-only error state, reports the missing HEAD and preserves all bytes for deliberate recovery. The user must never be auto-migrated to a new root and lose a clear reference to data. `AppDocument::storageReady` is exposed read-only to Qt, and `InteractionState::reload()` refuses to open or mutate SQLite when a confirmed definition cannot be loaded; the app shows its storage error rather than treating the bundled demo as the user's active app.

### Selection contract

- `historyEntries()` walks **only the ancestry of the current confirmed HEAD**, never numeric file order. Entries include generation, short description, UTC timestamp, digest and `current/legacy` flags.
- Each hop re-reads the generation JSON, validates the declarative schema and matching metadata if present, and checks the digest supplied by the confirmed child. Current modern HEAD must include metadata. The selection is bounded to 256 nodes and fails closed on broken linkage, missing files or changed current HEAD. Orphan generations are intentionally not listed or restorable.
- `previewRestoreGeneration(n)` re-verifies that **n is a confirmed ancestor of the current HEAD**, then stages the old interface for review. Restoring the active generation, a guessed ID or an orphan is rejected.
- To keep record schemas and actual user rows compatible with older UI views, restore preserves the **current app-owned model definitions** (existing field IDs and typed defaults); the older visual widgets/actions may be restored, not prior user rows. Restoring incompatible source/permission definitions is still rejected. Apply always creates a **new generation whose parent is the previously current HEAD**, not an in-place rewind.
- `rollback()` uses the same trusted selection pathway for the immediate parent. A legacy generation without metadata can be shown as a verified child-hash-anchored parent; a legacy HEAD with no known parent is not guessed.
- A read of a broken chain reports the error and leaves the current document/user data unchanged. No automatic deletion, rewriting, resync or disk scanning for a "best looking" arbitrary snapshot.

**Deliberate limit:** This is a **verified ancestry browser and fail-closed protection**, not yet a user-operated forensic recovery utility that recreates a missing/corrupt HEAD. A separate opt-in repair transaction with user confirmation and backup remains the next recovery sub-gate; automatic repair is unsafe.

## R0-03: ordinary-user readable preview

All preview routes (`previewDocument`, schema-validated AI mutation, local developer operations, selected historical restore) go through `AppDocument::stage()`. It now compares **all declared widget properties**, instead of a hand-maintained allowlist. This includes `defaultCity`, `model`, `filter`, `sort`, `style`, `options`, `message`, `source`, `icon` and the existing `action`, `parent`, `title` etc.

The summary includes component addition/removal/reordering, action/model impacts, added typed models and fields, changes to application metadata, a clearly labeled unchanged network-permission/source boundary, and a warning that interface restore never rewinds SQLite data. Objects/arrays are rendered as readable bounded JSON; long values are truncated to protect UI responsiveness. No raw user-record rows are included.

**Normal mode:** the actual human-readable summary is displayed before Apply — not only in developer mode. The secondary "История" dialog is accessible without Ctrl+Shift+D and its Restore buttons **only stage a preview**.

**Security invariants unchanged:** proposed sources/permissions must match the accepted document; additional access is rejected (not silently granted) before preview. Preview does not write SQLite, perform data migration or publish a generation. Apply rechecks the current HEAD and fails on stale writers.

## Test matrix

| Test name in `RuntimeTest.cpp` | What it proves in the Qt test fixture |
|---|---|
| `missingHeadFailsClosedWithoutDeletingFiles` | missing HEAD + generation, orphan-only folder and surviving private SQLite file all block bootstrap, preserve exact bytes, and leave `InteractionState` non-writable |
| `verifiedHistoryRestoreRejectsOrphansAndPreservesRecords` | orphan skipped in sequence 4→2→1; only confirmed root can be previewed; Apply creates a fresh generation; private SQLite task row survives; ancestor tampering is rejected |
| `semanticPreviewIncludesAllWidgetAndModelChanges` | style, filter, sort, options, message, source, icon, defaultCity, action, added field and permissions line are present; Cancel leaves HEAD unchanged; disallowed permission change rejected |
| `qmlPreviewAndTitles` (extended) | ordinary mode renders the actual semantic preview; the history dialog exposes a trusted ancestor and invokes *preview*, not immediate rollback |
| Historical regressions | orphan metadata, parent integrity, stale writer, SQL concurrency, preview cancel and city reactivity remain enabled |

### Validate (required Qt 6.12 kit)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/msvc2022_64 -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Static [contract CI](https://github.com/cybou-fr/mutyva/actions/workflows/contracts.yml) does not execute C++ or QML. If Qt 6.12 CI is blocked, record **IMPLEMENTED-UNVERIFIED**, do not check R0-02/R0-03 as fully complete.

## Remaining gates

- Real MSVC/Qt 6.12 CTest + screenshot and practical navigation testing.
- Dedicated user-approved forensic recovery for missing/corrupt HEAD with explicit verified chain choice and an immutable backup before repair.
- Improve preview presentation for very large diffs and test screen-reader/narrow-device output.
- Consider a future fully typed structured diff for translated/auditable UI. The current diff is human-readable, not a machine-actionable patch.
