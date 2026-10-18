#!/usr/bin/env python3
"""Verify packaged artwork and platform icon sizes without a native desktop session."""
from pathlib import Path
import hashlib
import json
import struct
import xml.etree.ElementTree as ET
from PIL import Image

root = Path(__file__).resolve().parents[1]
brand = root / "branding"
manifest = json.loads((brand / "exports/manifest.json").read_text(encoding="utf-8"))
for name, digest in manifest["sourcesSha256"].items():
    assert hashlib.sha256((brand / "source" / name).read_bytes()).hexdigest() == digest
with Image.open(brand / "icons/mutyva.ico") as icon:
    assert icon.ico.sizes() == {(n, n) for n in (16, 24, 32, 48, 64, 128, 256)}
icns = (brand / "icons/mutyva.icns").read_bytes()
assert icns[:4] == b"icns" and struct.unpack(">I", icns[4:8])[0] == len(icns)
with Image.open(brand / "icons/mutyva.icns") as icon:
    assert icon.size == (1024, 1024)
for n in (16, 24, 32, 48, 64, 128, 256, 512):
    with Image.open(brand / f"icons/{n}x{n}/mutyva.png") as icon:
        assert icon.size == (n, n)
for density, scale in (("mdpi", 1), ("hdpi", 1.5), ("xhdpi", 2), ("xxhdpi", 3), ("xxxhdpi", 4)):
    folder = root / "android/res" / f"mipmap-{density}"
    with Image.open(folder / "ic_launcher.png") as legacy:
        assert legacy.size == (int(48 * scale),) * 2
    with Image.open(folder / "ic_launcher_foreground.png") as image, Image.open(folder / "ic_launcher_monochrome.png") as mono:
        size = int(108 * scale)
        assert image.size == mono.size == (size, size)
        assert image.getchannel("A").tobytes() == mono.getchannel("A").tobytes()
        x0, y0, x1, y1 = image.getchannel("A").getbbox()
        radius = size * 33 / 108
        assert max(((x - size / 2) ** 2 + (y - size / 2) ** 2) ** .5
                   for x in (x0, x1) for y in (y0, y1)) <= radius + 1
for path in (root / "android").rglob("*.xml"):
    ET.parse(path)
app = ET.parse(root / "android/AndroidManifest.xml").getroot().find("application")
assert app.attrib["{http://schemas.android.com/apk/res/android}icon"] == "@mipmap/ic_launcher"
assert "Icon=mutyva" in (brand / "platform/org.mutyva.app.desktop").read_text(encoding="utf-8")
print("Branding OK: source hashes, ICO/ICNS/PNG sizes, adaptive safe zone, monochrome alpha and XML")
