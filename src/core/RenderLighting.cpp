#include "RenderLighting.hpp"

#include <array>
#include <cstdint>

namespace {

struct FaceAsset {
	double gain;
	std::array<double, 3> lift_bgr;
};

// Robust per-face medians from scripts/derive_shading.py. A single value per
// face preserves the render's directional lighting without transferring
// Steve-specific texture features or unsupported areas from the reference skin
// onto a user's skin.
constexpr std::array<FaceAsset, static_cast<size_t>(CubeFace::Count)> kFaces = {{
	{0.9970, {7.5885, 14.0075, 12.0680}},
	{1.0000, {0.9997, -6.0342, -3.0879}},
	{0.4009, {-10.2830, 0.3971, 0.1266}},
	{1.0545, {8.7213, 7.8726, 9.4449}},
	{1.0000, {0.7927, 4.6941, 18.5713}},
	{0.9710, {9.8789, 10.8916, 7.3598}},
	{0.7214, {4.7953, 4.1408, 1.6887}},
	{1.0609, {11.6821, 13.4165, 13.2647}},
	{0.2587, {-0.6126, 7.4075, 13.6229}},
	{1.0737, {3.7430, 3.6561, 2.8202}},
	{0.7392, {3.2368, -3.5256, -4.2845}},
	{1.0924, {3.8904, 3.9722, 3.4844}},
}};

} // namespace

void ApplyFaceLighting(cv::Mat& face_texture, CubeFace face, cv::Size uv_extent) {
	if (face_texture.empty() || face_texture.type() != CV_8UC4) {
		return;
	}
	if (face < CubeFace::HeadFront || face >= CubeFace::Count) {
		return;
	}

	(void)uv_extent;
	const FaceAsset& lighting = kFaces[static_cast<size_t>(face)];

	for (int y = 0; y < face_texture.rows; ++y) {
		uint8_t* row = face_texture.ptr<uint8_t>(y);
		for (int x = 0; x < face_texture.cols; ++x) {
			uint8_t* pixel = row + x * 4;
			for (int channel = 0; channel < 3; ++channel) {
				const double add = lighting.lift_bgr[channel] * (pixel[3] / 255.0);
				const double lit = pixel[channel] * lighting.gain + add;
				pixel[channel] = cv::saturate_cast<uint8_t>(lit);
			}
		}
	}
}
