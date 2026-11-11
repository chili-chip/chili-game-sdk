#!/usr/bin/env python3
"""Rebuilds the docs screenshots from the demo games.

Usage: python3 tools/screenshots.py [build dir]

Builds and runs tools/shots.cpp (Chili Catch) and tools/rpg_shots.cpp
(Pepper Village), then saves each frame 3x larger as
docs/assets/shots/<name>.png. The "rpg-splash" frame also becomes the RPG
demo's splash image. Needs Pillow.
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

for tool in ("chili-shots", "chili-rpg-shots"):
    subprocess.run(["cmake", "--build", str(build), "--target", tool], check=True)
    exe = next(p for p in build.rglob(tool) if p.is_file())

    with tempfile.TemporaryDirectory() as tmp:
        subprocess.run([str(exe), tmp], check=True)
        for ppm in sorted(pathlib.Path(tmp).glob("*.ppm")):
            img = Image.open(ppm)
            if ppm.stem == "rpg-splash":
                # the splash is 128x96, at 1x
                target = root / "examples" / "rpg" / "assets" / "splash.png"
                img.crop((0, 16, 128, 112)).save(target, optimize=True)
                print(target.relative_to(root))
                continue
            img = img.resize((img.width * 3, img.height * 3), Image.NEAREST)
            target = out / (ppm.stem + ".png")
            img.save(target, optimize=True)
            print(target.relative_to(root))
