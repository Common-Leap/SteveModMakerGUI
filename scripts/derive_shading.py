#!/usr/bin/env python3
"""Recover the render lighting used by Super Smash Bros. Ultimate's Steve art.

`Resources/chara_3_pickel_00.png` is the render Nintendo ships for Steve's first
costume slot.  Rendering the matching Minecraft skin through this tool's own
geometry gives the un-lit albedo for that exact image, so dividing the two
recovers the lighting the shipped art was drawn with, with no guesswork.

The recovered model, per cube face and in that face's own UV space, is

    out[channel] = clamp(albedo[channel] * gain(u, v) + lift_bgr[channel])

`lift_bgr` is a constant signed color offset and `gain` a robust, interpolated
texel grid. Both are properties of the pose and the lights, not of Steve, so
they transfer to any skin -- which is the whole point of solving for them in
UV space rather than baking a canvas-sized overlay.

Outputs `Resources/shading/<face>.png` (gain, uint16, 1.0 encoded as 10000) and
prints the lift table to paste into RenderLighting.cpp.

Requires opencv-python and numpy.  Run from the repository root:

    python3 scripts/derive_shading.py
"""
import hashlib
import os
import sys
import urllib.request

import cv2
import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(ROOT, 'Resources')
OUT = os.path.join(RES, 'shading')

# The historical bearded Steve texture used by the old promotional/Smash-era
# design. The current Mojang template is clean-shaven, so it is not a valid
# source for recovering the render's face lighting.
SKIN_URL = 'https://www.minecraftskins.com/uploads/skins/2012/06/21/skin_20120621193416166331.png'
SKIN_SHA256 = 'fa23b49a5dc72c0937eaef44ff1316854dfff9b003478f444554d2745493e0fb'

CANVAS = (968, 1864)
SUPER_SAMPLE = 2.0
DST = {
    'head': np.float32([[0, 0], [800, 0], [800, 800], [0, 800]]),
    '4x12': np.float32([[0, 0], [400, 0], [400, 1200], [0, 1200]]),
    '3x12': np.float32([[0, 0], [300, 0], [300, 1200], [0, 1200]]),
    'body': np.float32([[0, 0], [800, 0], [800, 1200], [0, 1200]]),
}

# Mirrors CreateRender()'s big-arms path: (name, crop rect, warp quad, part kind).
PARTS = [
    ('headfront',          (8, 8, 8, 8),    (366, 59, 776, 86, 774, 529, 368, 521),       'head'),
    ('headside',           (0, 8, 8, 8),    (210, 119, 366, 59, 368, 521, 212, 537),      'head'),
    ('headbottom',         (16, 0, 8, 8),   (366, 520, 774, 529, 591, 537, 212, 537),     'head'),
    ('layerheadside',      (32, 8, 8, 8),   (176, 94, 350, 23, 352, 547, 178, 564),       'head'),
    ('layerheadfront',     (40, 8, 8, 8),   (350, 23, 813, 56, 813, 548, 352, 547),       'head'),
    ('rightarmfront',      (44, 20, 4, 12), (120, 512, 328, 526, 305, 1194, 94, 1194),    '4x12'),
    ('leftarmfront',       (36, 52, 4, 12), (716, 532, 902, 527, 924, 1162, 740, 1170),   '4x12'),
    ('layerrightarmfront', (44, 36, 4, 12), (119, 496, 346, 513, 325, 1211, 92, 1213),    '4x12'),
    ('layerleftarmfront',  (52, 52, 4, 12), (709, 523, 919, 510, 944, 1163, 735, 1187),   '4x12'),
    ('rightarmside',       (40, 20, 4, 12), (51, 522, 120, 512, 94, 1194, 26, 1172),      '4x12'),
    ('leftarmside',        (32, 52, 4, 12), (627, 534, 715, 500, 740, 1171, 651, 1143),   '4x12'),
    ('layerrightarmside',  (40, 36, 4, 12), (34, 506, 119, 496, 92, 1213, 9, 1183),       '4x12'),
    ('layerleftarmside',   (48, 52, 4, 12), (611, 563, 709, 523, 735, 1187, 635, 1165),   '4x12'),
    ('bodyfront',          (20, 20, 8, 12), (325, 526, 725, 532, 722, 1175, 326, 1194),   'body'),
    ('bodyside',           (16, 20, 4, 12), (252, 534, 325, 526, 326, 1196, 249, 1167),   '4x12'),
    ('layerbodyfront',     (20, 36, 8, 12), (318, 504, 743, 512, 742, 1175, 323, 1196),   'body'),
    ('layerbodyside',      (16, 36, 4, 12), (241, 515, 318, 504, 323, 1196, 244, 1166),   '4x12'),
    ('rightlegside',       (0, 20, 4, 12),  (249, 1161, 327, 1194, 320, 1846, 244, 1793), '4x12'),
    ('rightlegfront',      (4, 20, 4, 12),  (327, 1194, 528, 1184, 519, 1825, 320, 1846), '4x12'),
    ('leftlegfront',       (20, 52, 4, 12), (528, 1184, 722, 1175, 720, 1801, 529, 1824), '4x12'),
    ('layerrightlegside',  (0, 36, 4, 12),  (239, 1183, 319, 1192, 311, 1864, 234, 1799), '4x12'),
    ('layerrightlegfront', (4, 36, 4, 12),  (319, 1192, 542, 1173, 524, 1847, 311, 1864), '4x12'),
    ('layerleftlegfront',  (4, 52, 4, 12),  (514, 1175, 741, 1157, 740, 1820, 524, 1847), '4x12'),
]

COMPOSITE_ORDER = [
    'leftarmside', 'headbottom', 'bodyside', 'layerleftarmside', 'bodyfront',
    'HEAD_SHADOW', 'layerbodyfront', 'layerbodyside', 'headfront', 'headside',
    'rightlegside', 'rightlegfront', 'rightarmside', 'rightarmfront',
    'leftlegfront', 'leftarmfront', 'layerheadfront', 'layerheadside',
    'LEG_SHADOW', 'layerrightlegside', 'layerrightarmfront', 'layerrightarmside',
    'layerrightlegfront', 'layerleftlegfront', 'layerleftarmfront',
]

# Each cube face, and the base-layer part whose visible pixels sample it. The
# overlay ("layer") parts reuse their base face: same plane, same lighting.
FACES = {
    'head_front': 'headfront', 'head_side': 'headside', 'head_bottom': 'headbottom',
    'body_front': 'bodyfront', 'body_side': 'bodyside',
    'arm_right_front': 'rightarmfront', 'arm_right_side': 'rightarmside',
    'arm_left_front': 'leftarmfront', 'arm_left_side': 'leftarmside',
    'leg_right_front': 'rightlegfront', 'leg_right_side': 'rightlegside',
    'leg_left_front': 'leftlegfront',
}

PART = {n: (r, q, k) for n, r, q, k in PARTS}
FACE_FOR_PART = {part: face for face, part in FACES.items()}
FACE_FOR_PART.update({f'layer{part}': face for face, part in FACES.items()})

GAIN_SCALE = 10000.0  # gain -> uint16; must match kGainScale in RenderLighting.cpp
MAX_GAIN = 255.0 / 200.0
MAX_LIFT = 20.0
STORE_DIV = 8         # gain maps are smooth, so store them at 1/8 UV resolution
MIN_SUPPORT = 20000.0  # below this a face is too occluded to fit a gradient to
EDGE_ALPHA_SIGMA = 1.9  # match ComposeRenderedSurface's silhouette coverage ramp


# --------------------------------------------------------------------------- #
# A minimal port of CreateRender(), enough to produce the un-lit albedo.
# --------------------------------------------------------------------------- #

def crop_scale(skin, x, y, w, h):
    return cv2.resize(skin[y:y + h, x:x + w], (w * 100, h * 100),
                      interpolation=cv2.INTER_NEAREST)


def warp(quad, kind, img):
    scaled_quad = np.float32(quad).reshape(4, 2) * SUPER_SAMPLE
    m = cv2.getPerspectiveTransform(scaled_quad, DST[kind])
    # Match RenderPerspectiveTransformation(): interpolate premultiplied color
    # so transparent texels do not create a black fringe along the silhouette.
    source = img.astype(np.float32) / 255.0
    source[:, :, :3] *= source[:, :, 3:4]
    warped = cv2.warpPerspective(
        source, m,
        (int(CANVAS[0] * SUPER_SAMPLE), int(CANVAS[1] * SUPER_SAMPLE)),
        flags=cv2.INTER_LINEAR | cv2.WARP_INVERSE_MAP,
        borderMode=cv2.BORDER_CONSTANT,
        borderValue=(0, 0, 0, 0),
    )
    warped = cv2.resize(warped, CANVAS, interpolation=cv2.INTER_AREA)
    alpha = np.clip(warped[:, :, 3:4], 0.0, 1.0)
    output = np.zeros(warped.shape, dtype=np.uint8)
    output[:, :, 3] = np.rint(alpha[:, :, 0] * 255.0).clip(0, 255).astype(np.uint8)
    valid = alpha[:, :, 0] > 1.0e-6
    colors = np.divide(warped[:, :, :3], alpha,
                       out=np.zeros_like(warped[:, :, :3]), where=valid[:, :, None])
    output[:, :, :3] = np.rint(colors * 255.0).clip(0, 255).astype(np.uint8)
    return output


def unwarp(img, quad, kind, size):
    m = cv2.getPerspectiveTransform(np.float32(quad).reshape(4, 2), DST[kind])
    return cv2.warpPerspective(img, m, size, flags=cv2.INTER_LINEAR)


def overlay(bg, fg):
    """Straight-alpha 'over', matching AlphaBlendColors in ImageUtils.cpp."""
    a1 = fg[:, :, 3:4].astype(np.float32) / 255.0
    a2 = bg[:, :, 3:4].astype(np.float32) / 255.0
    na = ((a1 + a2 * (1.0 - a1)) * 255.0).astype(np.uint8).astype(np.float32)
    mult = np.where(na > 0, 255.0 / np.maximum(na, 1e-9), 0.0)
    rgb = (fg[:, :, :3].astype(np.float32) * a1
           + bg[:, :, :3].astype(np.float32) * a2 * (1.0 - a1)) * mult
    out = np.empty_like(bg)
    out[:, :, :3] = np.clip(rgb, 0, 255).astype(np.uint8)
    out[:, :, 3] = na[:, :, 0].astype(np.uint8)
    return out


def smooth_alpha(surface):
    """Match the runtime's final silhouette coverage antialiasing."""
    alpha = surface[:, :, 3].astype(np.float32) / 255.0
    blurred = cv2.GaussianBlur(
        surface[:, :, 3],
        (0, 0),
        EDGE_ALPHA_SIGMA,
        EDGE_ALPHA_SIGMA,
        borderType=cv2.BORDER_CONSTANT,
    ).astype(np.float32) / 255.0
    output = surface.copy()
    output[:, :, 3] = np.clip(np.rint((alpha + blurred - alpha * blurred) * 255.0), 0, 255).astype(np.uint8)
    return output


def albedo_render(skin):
    """The composite with no lighting applied, but with the cast shadows kept."""
    layers = {n: warp(q, k, crop_scale(skin, *r)) for n, r, q, k in PARTS}
    for name in ('HEAD_SHADOW', 'LEG_SHADOW'):
        layers[name] = cv2.imread(os.path.join(RES, name + '.png'), cv2.IMREAD_UNCHANGED)
    surface = np.zeros((CANVAS[1], CANVAS[0], 4), np.uint8)
    for name in COMPOSITE_ORDER:
        surface = overlay(surface, layers[name])
    return smooth_alpha(surface)


def visible_part_map(skin):
    """Which part ends up on top at each pixel; later parts win, as in the composite."""
    names = [p[0] for p in PARTS]
    pid = np.zeros((CANVAS[1], CANVAS[0]), np.int32)
    for name in COMPOSITE_ORDER:
        if name not in names:
            continue
        r, q, k = PART[name]
        pid[warp(q, k, crop_scale(skin, *r))[:, :, 3] > 250] = names.index(name) + 1
    return pid, names


# --------------------------------------------------------------------------- #
# Solving for the lighting
# --------------------------------------------------------------------------- #

def fit_lift(a, o, mask):
    """Robust affine fit of official = albedo * gain + lift across a whole face.

    A face's texels span a range of albedo, which is what makes the constant
    term separable: a pure scale cannot explain dark texels being lifted
    proportionally more than bright ones, and they are.
    """
    A, O = a[mask].ravel(), o[mask].ravel()
    keep = np.ones_like(A, bool)
    coef = np.array([1.0, 0.0])
    for _ in range(6):
        m = np.stack([A[keep], np.ones(int(keep.sum()))], 1)
        coef, *_ = np.linalg.lstsq(m, O[keep], rcond=None)
        resid = np.abs(A * coef[0] + coef[1] - O)
        keep = resid < 3.0 * max(np.median(resid), 1.0)
    return float(np.clip(coef[1], 0.0, MAX_LIFT))


def fit_gain(uv_gain, uv_weight, nx, ny):
    """Recover one robust lighting sample for every source skin texel.

    A polynomial is too restrictive for the broad, low-frequency light rolloff
    on the shipped render, while an individual canvas-pixel fit follows warp
    and antialiasing noise.  Collapse the samples back to the source texel
    grid, use a median to reject edge pixels and texture-specific outliers, and
    interpolate that small grid over the face.  The result stays tied to UV
    position and transfers to arbitrary skins without embedding the reference.
    """
    h, w = uv_gain.shape
    med = np.ones((ny, nx), np.float32)
    sup = np.zeros((ny, nx), np.float32)
    for ty in range(ny):
        for tx in range(nx):
            y0, y1 = ty * h // ny, (ty + 1) * h // ny
            x0, x1 = tx * w // nx, (tx + 1) * w // nx
            block = uv_gain[y0:y1, x0:x1].ravel()
            good = uv_weight[y0:y1, x0:x1].ravel() > 0.5
            if good.sum() >= 0.25 * block.size:
                med[ty, tx], sup[ty, tx] = np.median(block[good]), good.mean()

    if not (sup > 0).any():
        return np.ones((h, w), np.float32)

    # Missing texels occur only on faces that are partly hidden.  Keep them at
    # neutral lighting; the caller already falls back to a constant when a
    # whole face has too little support.
    return cv2.resize(med, (w, h), interpolation=cv2.INTER_LINEAR)


def lit_render(skin, gains, lifts):
    """Render the classic reference using the same quantized maps as C++."""
    layers = {}
    for name, rect, quad, kind in PARTS:
        texture = crop_scale(skin, *rect)
        gain = np.round(gains[FACE_FOR_PART[name]] * GAIN_SCALE) / GAIN_SCALE
        gain = cv2.resize(gain, texture.shape[1::-1], interpolation=cv2.INTER_CUBIC)
        alpha = texture[:, :, 3:4].astype(np.float32) / 255.0
        lit = texture.copy()
        for channel in range(3):
            values = texture[:, :, channel].astype(np.float32) * gain
            lift = lifts[FACE_FOR_PART[name]]
            if not np.isscalar(lift):
                lift = lift[channel]
            values += lift * alpha[:, :, 0]
            lit[:, :, channel] = np.clip(np.rint(values), 0, 255).astype(np.uint8)
        layers[name] = warp(quad, kind, lit)

    for name in ('HEAD_SHADOW', 'LEG_SHADOW'):
        layers[name] = cv2.imread(os.path.join(RES, name + '.png'), cv2.IMREAD_UNCHANGED)

    surface = np.zeros((CANVAS[1], CANVAS[0], 4), np.uint8)
    for name in COMPOSITE_ORDER:
        surface = overlay(surface, layers[name])
    return smooth_alpha(surface)


def optimize_lifts(skin, gains, lifts, official):
    """Calibrate affine offsets against the final composite, not isolated faces.

    A face can be partly hidden by a layer or a cast shadow, so fitting its
    offset from isolated canvas samples leaves a small bias in the finished
    image.  Two bounded least-squares passes through the actual composite
    remove that bias while keeping the gain maps and the reference skin fixed.
    """
    visible = official[:, :, 3] > 0
    indices = np.flatnonzero(visible)
    faces = list(FACES)
    current_lifts = dict(lifts)
    current = lit_render(skin, gains, current_lifts)

    def rgb_values(image):
        return image.reshape(-1, 4)[indices, :3].reshape(-1).astype(np.float32)

    def score(image):
        return float(np.abs(rgb_values(image) - rgb_values(official)).mean())

    for _ in range(2):
        baseline = rgb_values(current)
        target = rgb_values(official) - baseline
        jacobian = np.empty((target.size, len(faces)), np.float32)
        for column, face in enumerate(faces):
            trial_lifts = dict(current_lifts)
            trial_lifts[face] += 1.0
            jacobian[:, column] = rgb_values(lit_render(skin, gains, trial_lifts)) - baseline

        delta, *_ = np.linalg.lstsq(jacobian, target, rcond=None)
        accepted = False
        baseline_score = score(current)
        for scale in (1.0, 0.5, 0.25, 0.125):
            trial_lifts = {
                face: float(np.clip(current_lifts[face] + delta[column] * scale,
                                    -MAX_LIFT, MAX_LIFT))
                for column, face in enumerate(faces)
            }
            trial = lit_render(skin, gains, trial_lifts)
            if score(trial) < baseline_score:
                current_lifts, current, accepted = trial_lifts, trial, True
                break
        if not accepted:
            break
    return current_lifts


def optimize_lift_tints(skin, gains, lifts, official):
    """Recover a small per-face BGR offset for the reference's colored light."""
    visible = official[:, :, 3] > 0
    indices = np.flatnonzero(visible)
    faces = list(FACES)
    current_lifts = {
        face: np.full(3, lift, np.float32) for face, lift in lifts.items()
    }
    current = lit_render(skin, gains, current_lifts)

    def rgb_values(image):
        return image.reshape(-1, 4)[indices, :3].reshape(-1).astype(np.float32)

    def score(image):
        return float(np.abs(rgb_values(image) - rgb_values(official)).mean())

    baseline = rgb_values(current)
    target = rgb_values(official) - baseline
    jacobian = np.empty((target.size, len(faces) * 3), np.float32)
    column = 0
    for face in faces:
        for channel in range(3):
            trial_lifts = {key: value.copy() for key, value in current_lifts.items()}
            trial_lifts[face][channel] += 1.0
            jacobian[:, column] = rgb_values(lit_render(skin, gains, trial_lifts)) - baseline
            column += 1

    delta, *_ = np.linalg.lstsq(jacobian, target, rcond=None)
    baseline_score = score(current)
    for scale in (1.0, 0.5, 0.25, 0.125):
        trial_lifts = {}
        column = 0
        for face in faces:
            trial_lifts[face] = np.clip(
                current_lifts[face] + delta[column:column + 3] * scale,
                -MAX_LIFT,
                MAX_LIFT,
            ).astype(np.float32)
            column += 3
        trial = lit_render(skin, gains, trial_lifts)
        if score(trial) < baseline_score:
            return trial_lifts
    return current_lifts


def derive(skin):
    alb = albedo_render(skin).astype(np.float32)
    official = cv2.imread(os.path.join(RES, 'chara_3_pickel_00.png'), cv2.IMREAD_UNCHANGED)
    if official is None:
        sys.exit('Resources/chara_3_pickel_00.png is missing')
    official = official.astype(np.float32)
    a, o = alb[:, :, :3], official[:, :, :3]

    # Trust a pixel only where the albedo is bright enough to divide by and is
    # not clipping at the top, and where both surfaces are fully opaque.
    luma = a @ np.float32([0.114, 0.587, 0.299])
    weight = (np.clip((luma - 18.0) / 40.0, 0, 1)
              * np.clip((250.0 - a.max(axis=2)) / 10.0, 0, 1))
    weight[(alb[:, :, 3] < 254) | (official[:, :, 3] < 254)] = 0

    pid, names = visible_part_map(skin)
    gains, lifts = {}, {}
    for face, part_name in FACES.items():
        rect, quad, kind = PART[part_name]
        size, nx, ny = (rect[2] * 100, rect[3] * 100), rect[2], rect[3]
        mask = cv2.erode((pid == names.index(part_name) + 1).astype(np.uint8),
                         np.ones((5, 5), np.uint8)) > 0   # drop antialiased borders
        sampled = mask & (weight > 0.5)

        lift = fit_lift(a, o, sampled) if sampled.sum() > 3000 else 0.0
        gain = ((a * (o - lift)).sum(2) / np.maximum((a * a).sum(2), 1e-6)).astype(np.float32)

        uv_gain = unwarp(gain, quad, kind, size)
        uv_weight = unwarp((weight * mask).astype(np.float32), quad, kind, size)
        uv_weight = np.where(uv_weight > 0.5, uv_weight, 0)
        if uv_weight.sum() < MIN_SUPPORT:
            # Barely visible face (the body and left-arm sides, the head's
            # underside): a single robust constant is all the data supports.
            seen = uv_weight > 0.05
            fitted = np.full(uv_gain.shape,
                             np.median(uv_gain[seen]) if seen.any() else 1.0, np.float32)
        else:
            fitted = fit_gain(uv_gain, uv_weight, nx, ny)

        fitted = np.clip(fitted, 0.02, MAX_GAIN)
        gains[face] = cv2.resize(fitted, (size[0] // STORE_DIV, size[1] // STORE_DIV),
                                 interpolation=cv2.INTER_AREA)
        lifts[face] = lift
    lifts = optimize_lifts(skin, gains, lifts, official)
    return gains, optimize_lift_tints(skin, gains, lifts, official)


def load_skin():
    path = os.path.join(os.environ.get('TMPDIR', '/tmp'), 'steve_classic_skin.png')
    if not os.path.exists(path):
        print(f'downloading reference skin: {SKIN_URL}')
        with urllib.request.urlopen(SKIN_URL, timeout=30) as r:
            data = r.read()
        with open(path, 'wb') as f:
            f.write(data)
    with open(path, 'rb') as f:
        data = f.read()
    digest = hashlib.sha256(data).hexdigest()
    if SKIN_SHA256 and digest != SKIN_SHA256:
        sys.exit(f'reference skin does not match the pinned texture (sha256 {digest})')
    print(f'  sha256 {digest}')
    skin = cv2.imread(path, cv2.IMREAD_UNCHANGED)
    if skin is None or skin.shape[:2] != (64, 64):
        sys.exit('reference skin must be a 64x64 RGBA png')
    return skin


def main():
    gains, lifts = derive(load_skin())
    os.makedirs(OUT, exist_ok=True)
    for face, gain in sorted(gains.items()):
        enc = np.clip(np.round(gain * GAIN_SCALE), 0, 65535).astype(np.uint16)
        cv2.imwrite(os.path.join(OUT, face + '.png'), enc)
        lift = lifts[face]
        if np.isscalar(lift):
            lift_text = f'{lift:5.2f}'
        else:
            lift_text = 'BGR ' + '/'.join(f'{value:5.2f}' for value in lift)
        print(f'{face:16} {enc.shape[1]:>3}x{enc.shape[0]:<3} '
              f'gain {gain.min():.3f}..{gain.max():.3f}  lift {lift_text}')
    print('\n// lift table for RenderLighting.cpp')
    for face, lift in sorted(lifts.items()):
        if np.isscalar(lift):
            print(f'    {lift:.4f},  // {face}')
        else:
            print('    {{{}}},  // {}'.format(
                ', '.join(f'{value:.4f}' for value in lift), face))


if __name__ == '__main__':
    main()
