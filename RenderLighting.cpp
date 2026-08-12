#include "RenderLighting.hpp"

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>

#include "EmbeddedAssets.hpp"

namespace {

// Gain maps are stored as 16-bit grayscale with 1.0 encoded as this value,
// while the derivation clips useful gain values at 255 / 200 = 1.275. Must match
// GAIN_SCALE in
// scripts/derive_shading.py.
constexpr double kGainScale = 10000.0;

struct FaceAsset {
	const char* key;
	std::array<double, 3> lift_bgr;
};

// Solved for by scripts/derive_shading.py against Resources/chara_3_pickel_00.png.
constexpr std::array<FaceAsset, static_cast<size_t>(CubeFace::Count)> kFaces = {{
	{"Resources/shading/head_front.png", {7.5885, 14.0075, 12.0680}},
	{"Resources/shading/head_side.png", {0.9997, -6.0342, -3.0879}},
	{"Resources/shading/head_bottom.png", {-10.2830, 0.3971, 0.1266}},
	{"Resources/shading/body_front.png", {8.7213, 7.8726, 9.4449}},
	{"Resources/shading/body_side.png", {0.7927, 4.6941, 18.5713}},
	{"Resources/shading/arm_right_front.png", {9.8789, 10.8916, 7.3598}},
	{"Resources/shading/arm_right_side.png", {4.7953, 4.1408, 1.6887}},
	{"Resources/shading/arm_left_front.png", {11.6821, 13.4165, 13.2647}},
	{"Resources/shading/arm_left_side.png", {-0.6126, 7.4075, 13.6229}},
	{"Resources/shading/leg_right_front.png", {3.7430, 3.6561, 2.8202}},
	{"Resources/shading/leg_right_side.png", {3.2368, -3.5256, -4.2845}},
	{"Resources/shading/leg_left_front.png", {3.8904, 3.9722, 3.4844}},
}};

// The stored maps are compact UV grids, so decode each once and scale it up to
// whatever size the caller's face texture happens to be.
const cv::Mat& GainMap(CubeFace face) {
	static std::array<cv::Mat, static_cast<size_t>(CubeFace::Count)> cache;

	const size_t index = static_cast<size_t>(face);
	cv::Mat& slot = cache[index];
	if (slot.empty()) {
		const char* key = kFaces[index].key;
		cv::Mat decoded = EmbeddedAssets::LoadPng(key);
		if (decoded.empty()) {
			throw std::runtime_error(std::string("Missing embedded lighting asset: ") + key);
		}
		if (decoded.channels() != 1) {
			cv::cvtColor(decoded, decoded, decoded.channels() == 4 ? cv::COLOR_BGRA2GRAY : cv::COLOR_BGR2GRAY);
		}
		decoded.convertTo(slot, CV_32F, 1.0 / kGainScale);
	}
	return slot;
}

} // namespace

void ApplyFaceLighting(cv::Mat& face_texture, CubeFace face, cv::Size uv_extent) {
	if (face_texture.empty() || face_texture.type() != CV_8UC4) {
		return;
	}
	if (face < CubeFace::HeadFront || face >= CubeFace::Count) {
		return;
	}

	if (uv_extent.width < face_texture.cols || uv_extent.height < face_texture.rows) {
		uv_extent = face_texture.size();
	}

	cv::Mat gain;
	cv::resize(GainMap(face), gain, uv_extent, 0, 0, cv::INTER_CUBIC);
	if (gain.size() != face_texture.size()) {
		gain = gain(cv::Rect(0, 0, face_texture.cols, face_texture.rows));
	}
	const std::array<double, 3>& lift = kFaces[static_cast<size_t>(face)].lift_bgr;

	for (int y = 0; y < face_texture.rows; ++y) {
		uint8_t* row = face_texture.ptr<uint8_t>(y);
		const float* gain_row = gain.ptr<float>(y);
		for (int x = 0; x < face_texture.cols; ++x) {
			uint8_t* pixel = row + x * 4;
			const double scale = gain_row[x];
			for (int channel = 0; channel < 3; ++channel) {
				const double add = lift[channel] * (pixel[3] / 255.0);
				const double lit = pixel[channel] * scale + add;
				pixel[channel] = cv::saturate_cast<uint8_t>(lit);
			}
		}
	}
}
