#!/usr/bin/env python3
"""Create the cape-aware animation variants shipped with the cape templates.

Run this with the Smash Ultimate Blender add-on's Python dependencies on
``PYTHONPATH``. The source body directory is an extracted vanilla Pickel motion
directory. Only the explicitly listed animations are copied to the cape
template directories.

Example::

    PYTHONPATH=/path/to/smash-ultimate-blender/dependencies \\
      python3 scripts/generate_cape_animation_assets.py \\
      --source-body-dir /path/to/fighter/pickel/motion/body/c00 \\
      --output Templates/small_arms_cape --output Templates/big_arms_cape
"""

from __future__ import annotations

import argparse
from pathlib import Path
import sys

from ssbh_data_py import anim_data


CAPE_MESH_NAME = "CapeMesh"

# These motions are the only ones that need a CapeMesh visibility track. The
# default and victory motions show the cape; the recovery motions hide it while
# Steve's Elytra is attached. In particular, result-screen animations cannot
# safely fall back to a motion that has no CapeMesh track.
VISIBLE_ANIMATIONS = (
    "a00defaulteyelid.nuanmb",
    "j02win1.nuanmb",
    "j02win1wait.nuanmb",
    "j02win2.nuanmb",
    "j02win2wait.nuanmb",
    "j02win3.nuanmb",
    "j02win3wait.nuanmb",
    "j02lose.nuanmb",
)
HIDDEN_ANIMATIONS = (
    "d02specialhistart.nuanmb",
    "d02specialairhistart.nuanmb",
    "d02specialairhi.nuanmb",
    "d02specialairhimax.nuanmb",
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-body-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, action="append", required=True)
    try:
        separator = sys.argv.index("--")
    except ValueError:
        return parser.parse_args()
    return parser.parse_args(sys.argv[separator + 1 :])


def set_constant_visibility(anim, visible: bool) -> None:
    visibility_group = next(
        (group for group in anim.groups if group.group_type == anim_data.GroupType.Visibility),
        None,
    )
    if visibility_group is None:
        visibility_group = anim_data.GroupData(anim_data.GroupType.Visibility)
        anim.groups.append(visibility_group)

    node = next((node for node in visibility_group.nodes if node.name == CAPE_MESH_NAME), None)
    if node is None:
        node = anim_data.NodeData(CAPE_MESH_NAME)
        visibility_group.nodes.append(node)

    track = next((track for track in node.tracks if track.name == "Visibility"), None)
    if track is None:
        track = anim_data.TrackData(
            "Visibility", False, anim_data.TransformFlags(), [visible]
        )
        node.tracks.append(track)
    else:
        track.values = [visible]


def write_variants(source: Path, outputs: list[Path], names: tuple[str, ...], visible: bool) -> None:
    for name in names:
        input_path = source / name
        if not input_path.is_file():
            raise FileNotFoundError(f"Missing source animation: {input_path}")
        for output in outputs:
            output.mkdir(parents=True, exist_ok=True)
            animation = anim_data.read_anim(str(input_path))
            set_constant_visibility(animation, visible)
            animation.save(str(output / name))


def main() -> None:
    args = parse_args()
    source = args.source_body_dir.resolve()
    outputs = [output.resolve() for output in args.output]
    write_variants(source, outputs, VISIBLE_ANIMATIONS, True)
    write_variants(source, outputs, HIDDEN_ANIMATIONS, False)


if __name__ == "__main__":
    main()
