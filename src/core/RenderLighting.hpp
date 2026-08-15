#pragma once

#include <opencv2/opencv.hpp>

// The twelve cube faces of the player model that the character render can show.
// The overlay ("layer") parts share their base part's face: same plane, so the
// same lighting.
enum class CubeFace : int {
	HeadFront = 0,
	HeadSide,
	HeadBottom,
	BodyFront,
	BodySide,
	ArmRightFront,
	ArmRightSide,
	ArmLeftFront,
	ArmLeftSide,
	LegRightFront,
	LegRightSide,
	LegLeftFront,
	Count
};

// Applies the lighting recovered from Nintendo's shipped Steve render to one
// face's texture, in that face's own UV space:
//
//     out[channel] = clamp(albedo[channel] * gain(u, v) + lift_bgr[channel])
//
// Alpha is preserved, and the signed BGR offsets are weighted by it so
// transparent texels of the overlay layers stay transparent. Solving in UV
// space rather than canvas space is what lets one set of maps serve both the
// classic and slim models, and the base and overlay layers alike.
//
// `uv_extent` is the size of the quad the texture will be warped into. It only
// differs from the texture's own size for the slim model's three-texel-wide arm
// fronts, which are warped against a four-texel quad and so cover just part of
// it; passing it keeps the gain aligned to the quad rather than to the texture.
// Leave it empty to use the texture's size.
//
// See scripts/derive_shading.py for how the maps were solved for.
void ApplyFaceLighting(cv::Mat& face_texture, CubeFace face, cv::Size uv_extent = cv::Size());
