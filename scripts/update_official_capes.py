#!/usr/bin/env python3
"""Download the checked-in official Minecraft cape atlases.

The catalog and texture hashes live in src/core/CapeCatalog.cpp. This script downloads
the corresponding PNGs from Mojang's texture service and verifies every atlas
is the canonical 64x32 size used by Minecraft Java Edition.

    python3 scripts/update_official_capes.py
"""

from __future__ import annotations

import re
import struct
from pathlib import Path
from urllib.request import Request, urlopen


ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "src" / "core" / "CapeCatalog.cpp"
OUTPUT = ROOT / "Resources" / "capes"
ENTRY = re.compile(
	r'^\s*\{"(?P<id>[a-z0-9-]+)",\s*"(?P<name>[^"]+)",\s*"(?P<hash>[0-9a-f]{32,64})"\},\s*$',
    re.MULTILINE,
)
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"


def png_size(data: bytes) -> tuple[int, int]:
    if len(data) < 24 or data[:8] != PNG_SIGNATURE or data[12:16] != b"IHDR":
        raise ValueError("download is not a PNG image")
    return struct.unpack(">II", data[16:24])


def main() -> None:
    catalog_text = CATALOG.read_text(encoding="utf-8")
    entries = list(ENTRY.finditer(catalog_text))
    if not entries:
        raise SystemExit(f"no cape entries found in {CATALOG}")
    catalog_entry_lines = [line for line in catalog_text.splitlines() if '{"' in line]
    if len(entries) != len(catalog_entry_lines):
        raise SystemExit(f"could not parse every cape entry in {CATALOG}")
    ids = [entry.group("id") for entry in entries]
    if len(ids) != len(set(ids)):
        raise SystemExit(f"duplicate cape ID in {CATALOG}")

    OUTPUT.mkdir(parents=True, exist_ok=True)
    expected_files: set[Path] = set()
    for entry in entries:
        cape_id = entry.group("id")
        texture_hash = entry.group("hash")
        destination = OUTPUT / f"{cape_id}.png"
        expected_files.add(destination)
        url = f"https://textures.minecraft.net/texture/{texture_hash}"
        request = Request(url, headers={"User-Agent": "SteveModMaker/1.0"})
        with urlopen(request, timeout=30) as response:
            data = response.read()
        dimensions = png_size(data)
        if dimensions != (64, 32):
            raise SystemExit(
                f"{cape_id}: expected a 64x32 atlas, received {dimensions[0]}x{dimensions[1]}"
            )
        destination.write_bytes(data)
        print(f"wrote {destination.relative_to(ROOT)}")

    for stale in OUTPUT.glob("*.png"):
        if stale not in expected_files:
            stale.unlink()
            print(f"removed stale {stale.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
