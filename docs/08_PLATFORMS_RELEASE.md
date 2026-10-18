# Platform Strategy and Release Gates

## One shared core

Qt 6/C++20 modules and application declarations are platform-neutral. QML presentation adapts using width/height/input mode; local paths come from `QStandardPaths`; platform-specific credentials use OS vault adapters.

| Platform | Development strategy | Gate |
|---|---|---|
| Windows 11 (first) | Qt Desktop kit, CMake/MSVC 2022 | Real data + AI auth + mutation + persistence |
| Android phones/tablets | Qt for Android, SDK/NDK/JDK matching Qt docs | Touch, screen sizes, network, OAuth loopback, suspend/resume |
| Linux | Qt desktop kit | Fonts, packaging, secure vault availability and network |
| macOS | Qt/Xcode build on macOS | Signing, sandbox/network, OS vault |
| iOS/iPadOS | Qt for iOS via macOS/Xcode | Device authorization callback, secure vault, no executable code download |

Official maintained platform list: https://doc.qt.io/qt-6/supported-platforms.html

## Crucial reality checks

- Cross-platform **Qt support** does not mean an untested app has already shipped on all five OSes.
- The OpenAI OSS authorization flow specifies an exact HTTP loopback callback on `127.0.0.1`. Mobile browser/system restrictions need real-device testing, especially iOS. Do not invent an alternative redirect scheme without confirming OpenAI supports it.
- iOS cannot be built/published with only a Windows machine; macOS + Xcode are needed.
- Components are declarative compositions rendered by preinstalled code, avoiding dynamic code-loading problems on mobile stores.
- No cross-device sync in MVP. Same declaration may be exported/imported, but personal local data does not magically travel between devices.

## Release approach

Internal Windows proof → Android alpha → Linux/macOS verification → iOS validation → public OSS release. Do not turn CI or a complicated mobile packaging pipeline into a blocker for the very first runnable Qt demo.

## Minimum release checklist

Clean install; signed builds where applicable; no embedded secrets; license compliance; attribution; auth revoke/relogin; privacy explanation; user data deletion/export; UI reset/recovery; all known High security issues resolved.
