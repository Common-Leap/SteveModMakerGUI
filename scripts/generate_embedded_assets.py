#!/usr/bin/env python3
"""Regenerate EmbeddedAssets.cpp from the on-disk asset directories.

The CLI ships every resource it needs inside the binary so a release is a single
executable. This script turns the files under Resources/, Chara_Masks/ and
Templates/ back into that source file; run it after changing any of them.

    python3 scripts/generate_embedded_assets.py
"""
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUT = os.path.join(ROOT, 'EmbeddedAssets.cpp')

# Order matters only for readability; Find() is a linear scan over the table.
ASSET_DIRS = [
    ('Resources', ('.png',)),
    ('Resources/capes', ('.png',)),
    ('Chara_Masks', ('.png',)),
    ('Templates/big_arms', None),
    ('Templates/small_arms', None),
    ('Templates/big_arms_cape', None),
    ('Templates/small_arms_cape', None),
]

# Resources/ holds working files that the CLI never loads at runtime; keeping
# them out of the binary saves several megabytes.
EXCLUDE = {
    'Resources/chara_3_pickel_00.png',    # reference render, used by scripts/derive_shading.py
    'Chara_Masks/chara_3_pickel_00.png',  # chara_3 is written straight from the render
    'Chara_Masks/chara_3_pickel_01.png',
}

HEADER = '''#include "EmbeddedAssets.hpp"

#include <array>
#include <fstream>
#include <vector>

namespace {
'''

FOOTER = '''}  // namespace

namespace EmbeddedAssets {

const AssetView* Find(const std::string& key) {
    for (const Entry& e : kAssets) {
        if (key == e.key) {
            static AssetView view{};
            view.data = e.data;
            view.size = e.size;
            return &view;
        }
    }
    return nullptr;
}

cv::Mat LoadPng(const std::string& key) {
    const AssetView* asset = Find(key);
    if (!asset) {
        return cv::Mat();
    }
    std::vector<unsigned char> bytes(asset->data, asset->data + asset->size);
    return cv::imdecode(bytes, cv::IMREAD_UNCHANGED);
}

bool WriteFile(const std::string& key, const std::filesystem::path& output_path) {
    const AssetView* asset = Find(key);
    if (!asset) {
        return false;
    }
    std::filesystem::create_directories(output_path.parent_path());
    std::ofstream out(output_path, std::ios::binary);
    if (!out) {
        return false;
    }
    out.write(reinterpret_cast<const char*>(asset->data), static_cast<std::streamsize>(asset->size));
    return static_cast<bool>(out);
}

}  // namespace EmbeddedAssets
'''


def symbol(key):
    return ''.join(c if c.isalnum() else '_' for c in key)


def collect():
    assets = []
    for rel_dir, extensions in ASSET_DIRS:
        directory = os.path.join(ROOT, rel_dir)
        if not os.path.isdir(directory):
            sys.exit(f'missing asset directory: {rel_dir}')
        for name in sorted(os.listdir(directory)):
            path = os.path.join(directory, name)
            key = f'{rel_dir}/{name}'
            if not os.path.isfile(path) or key in EXCLUDE:
                continue
            if extensions and not name.endswith(extensions):
                continue
            with open(path, 'rb') as f:
                assets.append((key, f.read()))
    return assets


def emit(assets):
    out = [HEADER]
    for key, data in assets:
        name = symbol(key)
        out.append(f'static const unsigned char {name}[] = {{\n')
        for i in range(0, len(data), 12):
            chunk = ', '.join(f'0x{b:02x}' for b in data[i:i + 12])
            tail = '' if i + 12 >= len(data) else ','
            out.append(f'  {chunk}{tail}\n')
        out.append('};\n')
        out.append(f'static const unsigned int {name}_len = {len(data)};\n\n')

    out.append('struct Entry {\n')
    out.append('    const char* key;\n')
    out.append('    const unsigned char* data;\n')
    out.append('    size_t size;\n')
    out.append('};\n\n')
    out.append(f'static const std::array<Entry, {len(assets)}> kAssets = {{{{\n')
    for key, _ in assets:
        name = symbol(key)
        out.append(f'    {{"{key}", {name}, static_cast<size_t>({name}_len)}},\n')
    out.append('}};\n')
    out.append(FOOTER)
    return ''.join(out)


def main():
    assets = collect()
    text = emit(assets)
    with open(OUTPUT, 'w') as f:
        f.write(text)
    total = sum(len(d) for _, d in assets)
    print(f'wrote {OUTPUT}: {len(assets)} assets, {total / 1024:.0f} KiB of payload')


if __name__ == '__main__':
    main()
