#!/usr/bin/env python3
"""Deterministic format/size exports of the supplied artwork; no image generation.
Run: python tools/export_branding.py (requires Pillow).
"""
from pathlib import Path
import hashlib
import json
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
BRAND = ROOT / "branding"


def save_png(image, path):
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path, format="PNG", compress_level=9)


def fit(image, size):
    copy = image.copy()
    copy.thumbnail(size, Image.Resampling.LANCZOS)
    return copy


def centered(image, size):
    canvas = Image.new("RGBA", (size, size))
    canvas.alpha_composite(image, ((size - image.width) // 2, (size - image.height) // 2))
    return canvas


def main():
    icon = Image.open(BRAND / "source/app-icon.png").convert("RGBA")
    lockup = Image.open(BRAND / "source/logo-lockup.png").convert("RGBA")
    # Fixed source region isolates the original mark above the wordmark; pixels are not repainted.
    symbol = lockup.crop((400, 68, 1270, 655))
    exports = BRAND / "exports"
    exports.mkdir(parents=True, exist_ok=True)
    save_png(icon.resize((512, 512), Image.Resampling.LANCZOS), exports / "mutyva-icon.png")
    save_png(fit(lockup, (1200, 700)), exports / "mutyva-logo.png")
    save_png(centered(fit(symbol, (448, 448)), 512), exports / "mutyva-symbol.png")
    icons = BRAND / "icons"
    icons.mkdir(parents=True, exist_ok=True)
    icon.save(icons / "mutyva.ico", sizes=[(n, n) for n in (16, 24, 32, 48, 64, 128, 256)])
    icon.resize((1024, 1024), Image.Resampling.LANCZOS).save(icons / "mutyva.icns")
    for size in (16, 24, 32, 48, 64, 128, 256, 512):
        save_png(icon.resize((size, size), Image.Resampling.LANCZOS), icons / f"{size}x{size}/mutyva.png")
    for density, factor in (("mdpi", 1), ("hdpi", 1.5), ("xhdpi", 2), ("xxhdpi", 3), ("xxxhdpi", 4)):
        destination = ROOT / "android/res" / f"mipmap-{density}"
        save_png(icon.resize((int(48 * factor), int(48 * factor)), Image.Resampling.LANCZOS), destination / "ic_launcher.png")
        canvas = int(108 * factor)
        # A 54dp-wide mark (aspect ~1.5) fits the central 66dp safe circle.
        foreground = centered(fit(symbol, (int(54 * factor), int(54 * factor))), canvas)
        save_png(foreground, destination / "ic_launcher_foreground.png")
        mono = Image.new("RGBA", foreground.size, "white")
        mono.putalpha(foreground.getchannel("A"))
        save_png(mono, destination / "ic_launcher_monochrome.png")
    sources = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted((BRAND / "source").glob("*.png"))}
    manifest = {"sourcesSha256": sources, "symbolCrop": [400, 68, 1270, 655],
                "operations": ["source-region extraction", "Lanczos resampling", "format conversion", "adaptive-icon centering", "monochrome alpha mask"],
                "android": {"canvasDp": 108, "foregroundWidthDp": 54, "legacyDp": 48}}
    (BRAND / "exports/manifest.json").write_bytes((json.dumps(manifest, indent=2) + "\n").encode())
    print("MUTYVA branding exported: desktop, application and Android assets")


if __name__ == "__main__":
    main()
