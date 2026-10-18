# MUTYVA visual identity

![MUTYVA](exports/mutyva-logo.png)

**One App. Infinite Forms.** · **Одно приложение. Любая форма.**

The source artwork was supplied by the project owner in this chat. The originals are retained byte-for-byte. These are raster assets; no vector reconstruction, generative redraw or new artwork is claimed.

| Directory | Contents |
|---|---|
| `source/` | Original square application icon and transparent logo lockup |
| `exports/` | Runtime/documentation icon, logo, isolated symbol and provenance manifest |
| `icons/` | Windows multi-size ICO, macOS ICNS and Linux hicolor PNG sizes |
| `platform/` | Windows resource template and Linux desktop entry |
| `palette.json` | Shared in-app background, surface, text and accent colors |
| `../android/` | Qt Android manifest overlay, density assets, adaptive and themed icons |

The icon source retains its original black surroundings. The logo retains transparency. The mark is extracted from source coordinates `(400,68)-(1270,655)` above the wordmark; pixels are resampled, not repainted. Android uses that mark on a separate navy background, centered inside the adaptive safe region. The monochrome variant uses the same alpha mask.

## Reproduce and verify

Install Pillow in a development Python environment, then run:

```text
python tools/export_branding.py
python tools/verify_branding.py
```

The build consumes committed exports and does not require Pillow. The manifest records source SHA-256 hashes, the extraction region and export operations.

## Platform integration

- **Windows:** `cmake/Branding.cmake` embeds ICO and product/version metadata in the EXE. `QGuiApplication::setWindowIcon` supplies the running-window icon.
- **Linux:** `cmake --install` installs the executable, `org.mutyva.app.desktop` and PNGs in the hicolor hierarchy. Qt runtime libraries must be supplied by the target distribution/package.
- **macOS:** the bundle receives ICNS in `Contents/Resources` with the corresponding icon and bundle-name properties. A macOS build is required to verify Finder/Dock behavior.
- **Android:** the custom package directory overlays Qt's template. The launcher has legacy density images, adaptive foreground/background on API 26+, and a monochrome layer on API 33+. The manifest keeps Qt activity, library and FileProvider hooks.

The platform wiring follows [Qt application-icon guidance](https://doc.qt.io/qt-6.12/appicon.html), [Qt Android package overlays](https://doc.qt.io/qt-6/cmake-target-property-qt-android-package-source-dir.html) and [Android adaptive-icon guidance](https://developer.android.com/develop/ui/compose/system/icon_design_adaptive).

## Inside the app

The shared identity appears in the header, a first-run welcome dialog and **О MUTYVA**. Continue dismisses the welcome screen for subsequent launches; About remains accessible. Display naming is MUTYVA while the existing storage namespace is deliberately retained, so branding does not move or reset saved generations.

The artwork and palette do not enter the model-generated component grammar or grant any new action/permission. Welcome/About describe actual implemented capabilities and identify AI/auth as unfinished.

## Verification on this host

Windows MSVC/Qt 6.12 build passed; the EXE's icon resource and MUTYVA product/version metadata were inspected. Qt tests check PNG resources, header rendering, About, narrow welcome layout and dismissal without changing saved generations. ICO/ICNS sizes, source hashes, adaptive safe zone and monochrome alpha are checked by `verify_branding.py`.

Android SDK 36 AAPT2 successfully compiled and linked the custom launcher resources and manifest; badging reports MUTYVA and the adaptive icon. The resulting test archive contains resources only, not a runnable Qt application. A full Android build/device launch and native Linux/macOS packaging remain unverified on this Windows host.
