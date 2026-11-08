#!/usr/bin/env python3
"""Rebuilds the docs screenshots from the demo game.

Usage: python3 tools/screenshots.py [build dir]

Builds and runs tools/shots.cpp, then saves each frame 3x larger as
docs/assets/shots/<name>.png. Needs Pillow.
"""

import pathlib
import subprocess
import sys
import tempfile

from PIL import Image

root = pathlib.Path(__file__).resolve().parent.parent
build = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else root / "build"
out = root / "docs" / "assets" / "shots"
out.mkdir(parents=True, exist_ok=True)

subprocess.run(["cmake", "--build", str(build), "--target", "chili-shots"], check=True)
exe = next(build.rglob("chili-shots"))

with tempfile.TemporaryDirectory() as tmp:
    subprocess.run([str(exe), tmp], check=True)
    for ppm in sorted(pathlib.Path(tmp).glob("*.ppm")):
        img = Image.open(ppm)
        img = img.resize((img.width * 3, img.height * 3), Image.NEAREST)
        target = out / (ppm.stem + ".png")
        img.save(target, optimize=True)
        print(target.relative_to(root))
