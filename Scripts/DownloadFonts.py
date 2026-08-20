#!/usr/bin/env python3
"""Download the open-source font families used by Slate.

The files are runtime content, not generated source. The script is deliberately idempotent
and writes a LICENSE.txt beside every family.
"""
from pathlib import Path
from urllib.request import urlopen
from zipfile import ZipFile
from io import BytesIO
import shutil

ROOT = Path(__file__).resolve().parents[1] / "EngineContent" / "FontArchives"
FONTS = {
    "OpenSans": ("https://github.com/googlefonts/opensans/archive/refs/heads/main.zip", "OpenSans-main"),
    "Archivo": ("https://github.com/Omnibus-Type/Archivo/archive/refs/heads/main.zip", "Archivo-main"),
    "Inter": ("https://github.com/rsms/inter/archive/refs/heads/master.zip", "inter-master"),
    "JetBrainsMono": ("https://github.com/JetBrains/JetBrainsMono/archive/refs/heads/master.zip", "JetBrainsMono-master"),
}


def download(name, spec):
    destination = ROOT / name
    destination.mkdir(parents=True, exist_ok=True)
    if list(destination.glob("*.ttf")) or list(destination.glob("*.otf")):
        print(f"[skip] {name}: font files already exist")
        return
    url, root = spec
    print(f"[download] {name}")
    with urlopen(url, timeout=60) as response:
        archive = ZipFile(BytesIO(response.read()))
    candidates = [entry for entry in archive.namelist()
                  if entry.startswith(root + "/") and entry.lower().endswith((".ttf", ".otf"))]
    if not candidates:
        raise RuntimeError(f"no font files found in {url}")
    for entry in candidates:
        path = Path(entry)
        if any(part.lower() in {"static", "variable"} for part in path.parts) or "fonts" in path.parts:
            target = destination / path.name
            with archive.open(entry) as source, target.open("wb") as output:
                shutil.copyfileobj(source, output)
    (destination / "SOURCE.txt").write_text(
        f"Source: {url}\nLicense: see the upstream repository license.\n", encoding="utf-8")


ROOT.mkdir(parents=True, exist_ok=True)
for name, spec in FONTS.items():
    download(name, spec)
print(f"Fonts are available under {ROOT}")
