# Qt 6.12 native acceptance — reproduce and report

**Requirement:** Only **Qt 6.12.x** (not Qt 6.8, 6.11 or 6.13), Windows/MSVC 2022 and C++20 are supported as the Windows acceptance baseline. See the [CMake gate](../CMakeLists.txt) and [architecture quality requirements](23_ARCHITECTURE_ASSESSMENT.md).

**Status as of 2026-10-23:** historical Qt 6.8 Windows CTest and screenshot gallery passed; the **Qt 6.12** hosted pipeline is blocked during Qt Account credential availability checks. Until the native `QT612_PASS` report and a green Qt 6.12 workflow exist, **R0-02/R0-03 remain IMPLEMENTED-UNVERIFIED**, not fully accepted.

## Option A — Run the full gate locally without GitHub secrets

Install the official **open-source** Qt 6.12 MSVC 2022 x64 components on a Windows machine (Qt Quick / Quick Controls 2 / Qt SQL, Qt Network), and Visual Studio 2022 with Desktop C++ workload + CMake. The local installer can use your existing Qt Account; you never put credentials in this repository.

From a PowerShell terminal opened at the repository root:

```powershell
pwsh -NoProfile -File tools/verify-qt612.ps1 -QtRoot "C:\Qt\6.12.0\msvc2022_64"
```

The script checks `qmake QT_VERSION` is in `6.12.x`, clears a stale CMake cache, configures for MSVC x64, builds, executes **CTest with the complete offline Qt/QML runtime suite**, and separately captures 21 **fresh real QML screenshots** (must all be nonempty and have distinct SHA-256 hashes). It prints runtime test diagnostics and **throws on the first failed gate**, never falling back to another Qt release.

**Evidence created on success:** `qt612-evidence/qt612-acceptance.json` containing `QT612_PASS`, Qt version, MSVC kit, exact Git commit, UTC time, test exit code and screenshot names; `palette-captures/*.png` for visual inspection. These files do not contain OAuth tokens or user records. A generated report is **evidence of that execution**, not a cryptographic attestation.

If the script fails, preserve the terminal output, `build-ci/runtime-tests.txt` and the generated build logs. Do not mark roadmap tickets green based on a partially completed script or manually created JSON.

## Option B — Enable hosted GitHub Actions (recommended for repeatable evidence)

Repository maintainer must open the GitHub repository and add **two repository Actions secrets** in **Settings → Secrets and variables → Actions → New repository secret**:

- `QT_EMAIL` — email of a Qt Account authorized for Qt Open Source downloads.
- `QT_PW` — account password.

**Never** add secrets to a commit, an issue, ChatGPT, build arguments, logs or PR comments. On fork PRs, secrets may be unavailable by GitHub design. GitHub hosts the official Qt Online Installer via `jurplel/install-qt-action@v4` with `use-official: true` and exact Qt `6.12.0`, then calls the **same local native gate**. On success the CI publishes `mutyva-component-gallery` and `mutyva-qt612-acceptance` artifacts. Credentials are used only during CI installation; the resulting MUTYVA app never uses them.

After configuring secrets, launch or re-run [Windows Qt regression](https://github.com/cybou-fr/mutyva/actions/workflows/windows-qt.yml) on **main**. Confirm the job completed `success`, the logged Qt version is 6.12.x, CTest has 0 failures, all 21 screenshots were generated, and the acceptance report's commit matches the tested HEAD. A green static [contracts CI](https://github.com/cybou-fr/mutyva/actions/workflows/contracts.yml) **cannot substitute for this**.

## Acceptance boundary and consequences

- **If Qt 6.12 fails installation** with correct secrets, diagnose license/account, selected MSVC package/host, installer network, cache and installation metadata. Do not silently switch back to Qt 6.8.
- **If CMake/build fails**, treat it as an actual compatibility defect. Patch code and re-run all tests.
- **If CTest fails**, fix the specific Qt/QML runtime regression; do not skip or ignore failed tests.
- **If the gallery fails**, do not declare component rendering accepted; inspect genuine PNG output.
- **If both normal and negative R0-02/R0-03 tests pass** on exact Qt 6.12 and the repair/recovery requirements are addressed, update the linked roadmap ASRs with the run ID; until then retain open checkboxes.

Documentation: [R0 implementation and regression matrix](25_R0_RECOVERY_PREVIEW.md), [roadmap](06_IMPLEMENTATION_ROADMAP.md). This procedure deliberately supports independent repeatability without a custom backend or additional AI services.
