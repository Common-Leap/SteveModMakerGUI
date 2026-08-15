#include <algorithm>
#include <charconv>
#include <cctype>
#include <iostream>
#include <array>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "Nutexb.hpp"
#include "BNTX.hpp"
#include "CapeCatalog.hpp"
#include "Constants.hpp"
#include "EmbeddedAssets.hpp"
#include "ImageUtils.hpp"
#include "MinecraftSkinUtil.hpp"
#include "MsgNameTemplate.hpp"
#include "RenderLighting.hpp"
#include "UiCharaDbTemplate.hpp"

namespace {

cv::Mat LoadRequiredPng(const std::string& key) {
	cv::Mat img = EmbeddedAssets::LoadPng(key);
	if (img.empty()) {
		throw std::runtime_error("Missing embedded image asset: " + key);
	}
	return img;
}

bool WriteRequiredTemplate(
	const std::string& arm_type,
	bool cape_enabled,
	const std::string& file_name,
	const std::filesystem::path& output_path
) {
	const std::string arm_directory = (arm_type == "small") ? "small_arms" : "big_arms";
	const std::string template_dir = "Templates/" + arm_directory + (cape_enabled ? "_cape/" : "/");
	return EmbeddedAssets::WriteFile(template_dir + file_name, output_path);
}

bool WriteCapeArcropolisConfig(
	const std::string& slot_code,
	const std::filesystem::path& output_root
) {
	const std::filesystem::path output_path = output_root / "config.json";
	std::filesystem::create_directories(output_path.parent_path());
	std::ofstream out(output_path, std::ios::binary);
	if (!out) {
		return false;
	}

	const std::string costume = "c" + slot_code;
	std::vector<std::string> registered_files = {
		"fighter/pickel/model/body/" + costume + "/cape.nutexb",
		"fighter/pickel/motion/body/" + costume + "/swing.prc"
	};

	out
		<< "{\n"
		<< "  \"new-dir-files\": {\n"
		<< "    \"fighter/pickel/" << costume << "\": [\n";
	for (std::size_t index = 0; index < registered_files.size(); ++index) {
		out << "      \"" << registered_files[index] << "\"";
		if (index + 1 != registered_files.size()) {
			out << ',';
		}
		out << '\n';
	}
	out
		<< "    ]\n"
		<< "  }\n"
		<< "}\n";
	return static_cast<bool>(out);
}

std::string MsgNameSlotCode(int slot_input) {
	switch (slot_input) {
		case 1: return "08";
		case 2: return "09";
		case 3: return "02";
		case 4: return "03";
		case 5: return "04";
		case 6: return "05";
		case 7: return "06";
		case 8: return "07";
		default: return "02";
	}
}

char16_t ToUpperAscii(char16_t ch) {
	if (ch >= u'a' && ch <= u'z') {
		return static_cast<char16_t>(ch - (u'a' - u'A'));
	}
	return ch;
}

char16_t ToLowerAscii(char16_t ch) {
	if (ch >= u'A' && ch <= u'Z') {
		return static_cast<char16_t>(ch + (u'a' - u'A'));
	}
	return ch;
}

std::u16string AsciiBytesToUtf16(const std::string& input) {
	std::u16string result;
	result.reserve(input.size());
	for (unsigned char ch : input) {
		result.push_back(static_cast<char16_t>(ch));
	}
	return result;
}

bool IsAsciiText(const std::string& input) {
	for (unsigned char ch : input) {
		// Allow standard printable ASCII plus space.
		if (ch < 0x20 || ch > 0x7E) {
			return false;
		}
	}
	return true;
}

bool IsValidMinecraftUsername(const std::string& username) {
	if (username.empty() || username.size() > 16) {
		return false;
	}
	for (unsigned char ch : username) {
		if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
			(ch >= '0' && ch <= '9') || ch == '_')) {
			return false;
		}
	}
	return true;
}

bool TryParseSlotNumber(const char* raw_value, int& slot_out) {
	if (raw_value == nullptr || *raw_value == '\0') {
		return false;
	}

	const char* begin = raw_value;
	const char* end = raw_value + std::strlen(raw_value);
	const auto parsed = std::from_chars(begin, end, slot_out);
	return parsed.ec == std::errc() && parsed.ptr == end;
}

bool IsSafeRelativeOutputPath(const std::string& value) {
	if (value.empty()) {
		return false;
	}

	const std::filesystem::path path(value);
	if (path.empty() || path.is_absolute() || path.has_root_name() || path.has_root_directory()) {
		return false;
	}

	for (const auto& component : path) {
		if (component == "." || component == "..") {
			return false;
		}
	}
	return true;
}

std::u16string EscapeXmlText(const std::u16string& input) {
	std::u16string escaped;
	escaped.reserve(input.size());
	for (const char16_t ch : input) {
		switch (ch) {
		case u'&': escaped += u"&amp;"; break;
		case u'<': escaped += u"&lt;"; break;
		case u'>': escaped += u"&gt;"; break;
		case u'"': escaped += u"&quot;"; break;
		case u'\'': escaped += u"&apos;"; break;
		default: escaped.push_back(ch); break;
		}
	}
	return escaped;
}

bool WriteRenderPng(const cv::Mat& render, const std::filesystem::path& output_path) {
	if (render.empty() || render.type() != CV_8UC4 || output_path.empty()) {
		return false;
	}

	std::string extension = output_path.extension().string();
	std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) {
		return static_cast<char>(std::tolower(ch));
	});
	if (extension != ".png") {
		return false;
	}

	const std::filesystem::path parent = output_path.parent_path();
	if (!parent.empty()) {
		std::filesystem::create_directories(parent);
	}
	return cv::imwrite(output_path.string(), render);
}

std::u16string AsciiUpper(const std::u16string& input) {
	std::u16string result = input;
	for (char16_t& ch : result) {
		ch = ToUpperAscii(ch);
	}
	return result;
}

std::u16string AsciiLower(const std::u16string& input) {
	std::u16string result = input;
	for (char16_t& ch : result) {
		ch = ToLowerAscii(ch);
	}
	return result;
}

std::u16string AsciiCapitalized(const std::u16string& input) {
	if (input.empty()) {
		return input;
	}
	std::u16string result = input;
	result[0] = ToUpperAscii(result[0]);
	return result;
}

void ReplaceAll(std::u16string& text, const std::u16string& from, const std::u16string& to) {
	if (from.empty()) {
		return;
	}
	std::size_t pos = 0;
	while ((pos = text.find(from, pos)) != std::u16string::npos) {
		text.replace(pos, from.size(), to);
		pos += to.size();
	}
}

bool WriteMsgNameTemplate(
	const std::string& username,
	const std::string& special_message,
	bool slot_one,
	const std::string& msg_slot_code,
	const std::filesystem::path& output_path
) {
	const std::uint8_t* input_data = MsgNameTemplate::DataForSlot(slot_one);
	const std::size_t input_size = MsgNameTemplate::SizeForSlot(slot_one);
	if (input_data == nullptr || input_size < 2) {
		return false;
	}

	const bool has_utf16le_bom = (input_data[0] == 0xFF && input_data[1] == 0xFE);
	const std::size_t data_offset = has_utf16le_bom ? 2 : 0;
	if (((input_size - data_offset) % 2) != 0) {
		return false;
	}

	std::u16string text;
	text.reserve((input_size - data_offset) / 2);
	for (std::size_t i = data_offset; i + 1 < input_size; i += 2) {
		const std::uint16_t code_unit = static_cast<std::uint16_t>(input_data[i])
			| (static_cast<std::uint16_t>(input_data[i + 1]) << 8U);
		text.push_back(static_cast<char16_t>(code_unit));
	}

	// Slot 1 keeps a dedicated chr3 line that should always read "STEVE".
	// Inject it when missing from the bundled template.
	if (slot_one && text.find(u"label=\"nam_chr3_00_pickel\"") == std::u16string::npos) {
		const std::size_t closing_tag_pos = text.find(u"</xmsbt>");
		if (closing_tag_pos == std::u16string::npos) {
			return false;
		}
		const std::u16string chr3_entry =
			u"        <entry label=\"nam_chr3_00_pickel\">\n"
			u"                <text>STEVE</text>\n"
			u"        </entry>\n";
		text.insert(closing_tag_pos, chr3_entry);
	}

	// These values are inserted into XML text nodes. Escape them first so a
	// local display name such as "A&B" cannot make msg_name.xmsbt malformed.
	const std::u16string username_u16 = AsciiBytesToUtf16(username);
	const std::u16string message_u16 = AsciiBytesToUtf16(special_message.empty() ? username : special_message);
	const std::u16string slot_code_u16 = AsciiBytesToUtf16(msg_slot_code);
	const std::u16string slot_label_suffix = AsciiBytesToUtf16("_" + msg_slot_code + "_pickel\"");
	ReplaceAll(text, u"USERNAME", EscapeXmlText(AsciiUpper(username_u16)));
	ReplaceAll(text, u"Username", EscapeXmlText(AsciiCapitalized(username_u16)));
	ReplaceAll(text, u"username", EscapeXmlText(AsciiLower(username_u16)));
	ReplaceAll(text, u"Special-Message", EscapeXmlText(message_u16));
	// Only msg_name.xmsbt slot labels use this custom slot mapping.
	ReplaceAll(text, u"0x", slot_code_u16);
	ReplaceAll(text, u"0X", slot_code_u16);
	ReplaceAll(text, u"_00_pickel\"", slot_label_suffix);
	ReplaceAll(text, u"_02_pickel\"", slot_label_suffix);

	std::vector<std::uint8_t> output_bytes;
	output_bytes.reserve((has_utf16le_bom ? 2U : 0U) + text.size() * 2U);
	if (has_utf16le_bom) {
		output_bytes.push_back(0xFF);
		output_bytes.push_back(0xFE);
	}

	for (char16_t ch : text) {
		const std::uint16_t code_unit = static_cast<std::uint16_t>(ch);
		output_bytes.push_back(static_cast<std::uint8_t>(code_unit & 0x00FF));
		output_bytes.push_back(static_cast<std::uint8_t>((code_unit >> 8U) & 0x00FF));
	}

	std::filesystem::create_directories(output_path.parent_path());
	std::ofstream out(output_path, std::ios::binary);
	if (!out) {
		return false;
	}

	out.write(reinterpret_cast<const char*>(output_bytes.data()), static_cast<std::streamsize>(output_bytes.size()));
	return static_cast<bool>(out);
}

bool WriteSlotUiCharaDb(const std::string& slot_code, const std::filesystem::path& output_root) {
	const std::uint8_t* input_data = UiCharaDbTemplate::DataForSlot(slot_code);
	const std::size_t input_size = UiCharaDbTemplate::SizeForSlot(slot_code);
	if (input_data == nullptr || input_size == 0) {
		return false;
	}

	const std::filesystem::path output_file = output_root / "ui/param/database/ui_chara_db.prcx";
	std::filesystem::create_directories(output_file.parent_path());
	std::ofstream out(output_file, std::ios::binary);
	if (!out) {
		return false;
	}

	out.write(reinterpret_cast<const char*>(input_data), static_cast<std::streamsize>(input_size));
	return static_cast<bool>(out);
}

struct RenderCompositionInputs {
	cv::Mat headfront;
	cv::Mat headside;
	cv::Mat headbottom;
	cv::Mat layerheadside;
	cv::Mat layerheadfront;

	cv::Mat rightarmfront;
	cv::Mat leftarmfront;
	cv::Mat layerrightarmfront;
	cv::Mat layerleftarmfront;

	cv::Mat rightarmside;
	cv::Mat leftarmside;
	cv::Mat layerrightarmside;
	cv::Mat layerleftarmside;

	cv::Mat bodyfront;
	cv::Mat bodyside;
	cv::Mat layerbodyfront;
	cv::Mat layerbodyside;

	cv::Mat rightlegside;
	cv::Mat rightlegfront;
	cv::Mat leftlegfront;
	cv::Mat layerrightlegside;
	cv::Mat layerrightlegfront;
	cv::Mat layerleftlegfront;
};

cv::Mat CreateCapeRenderLayer(const cv::Mat& cape) {
	if (cape.empty() || cape.type() != CV_8UC4 || cape.cols < 11 || cape.rows < 17) {
		return cv::Mat();
	}

	// A worn cape is almost completely occluded in this tightly cropped,
	// front-facing pose. Show the decorated outward panel as a deliberate swatch
	// in the unused lower-left space instead of inventing a cape-shaped limb.
	const cv::Mat outward_panel = cape(cv::Rect(1, 1, 10, 16));
	cv::Mat preview;
	cv::resize(outward_panel, preview, cv::Size(180, 288), 0, 0, cv::INTER_NEAREST);
	cv::Mat framed;
	cv::copyMakeBorder(
		preview,
		framed,
		4, 4, 4, 4,
		cv::BORDER_CONSTANT,
		cv::Scalar(20, 20, 20, 220)
	);

	cv::Mat layer(1864, 968, CV_8UC4, cv::Scalar(0, 0, 0, 0));
	OverlayImage(layer, framed, cv::Point(24, 1450));
	return layer;
}

// Crops one cube face out of the skin, scales it up to render resolution, and
// applies that face's lighting while it is still in UV space -- see
// RenderLighting.hpp for why the lighting has to be applied before the warp.
cv::Mat LitFace(cv::Mat& skin, cv::Rect rect, CubeFace face, cv::Size uv_extent = cv::Size()) {
	cv::Mat texture = CropAndScale(skin, rect);
	ApplyFaceLighting(texture, face, uv_extent);
	return texture;
}

cv::Mat ComposeRenderedSurface(
	RenderCompositionInputs parts,
	const cv::Mat& head_shadow,
	const cv::Mat& leg_shadow,
	const cv::Mat& cape_layer
) {
	cv::Mat surface(1864, 968, CV_8UC4);
	surface.setTo(0);
	if (!cape_layer.empty()) {
		OverlayImage(surface, cape_layer, cv::Point(0, 0));
	}

	OverlayImage(surface, parts.leftarmside, cv::Point(0, 0));

	OverlayImage(surface, parts.headbottom, cv::Point(0, 0));
	OverlayImage(surface, parts.bodyside, cv::Point(0, 0));

	OverlayImage(surface, parts.layerleftarmside, cv::Point(0, 0));

	OverlayImage(surface, parts.bodyfront, cv::Point(0, 0));

	OverlayImage(surface, head_shadow, cv::Point(0, 0));

	OverlayImage(surface, parts.layerbodyfront, cv::Point(0, 0));
	OverlayImage(surface, parts.layerbodyside, cv::Point(0, 0));

	OverlayImage(surface, parts.headfront, cv::Point(0, 0));
	OverlayImage(surface, parts.headside, cv::Point(0, 0));

	OverlayImage(surface, parts.rightlegside, cv::Point(0, 0));
	OverlayImage(surface, parts.rightlegfront, cv::Point(0, 0));
	OverlayImage(surface, parts.rightarmside, cv::Point(0, 0));
	OverlayImage(surface, parts.rightarmfront, cv::Point(0, 0));
	OverlayImage(surface, parts.leftlegfront, cv::Point(0, 0));
	OverlayImage(surface, parts.leftarmfront, cv::Point(0, 0));

	OverlayImage(surface, parts.layerheadfront, cv::Point(0, 0));
	OverlayImage(surface, parts.layerheadside, cv::Point(0, 0));

	OverlayImage(surface, leg_shadow, cv::Point(0, 0));

	OverlayImage(surface, parts.layerrightlegside, cv::Point(0, 0));
	OverlayImage(surface, parts.layerrightarmfront, cv::Point(0, 0));
	OverlayImage(surface, parts.layerrightarmside, cv::Point(0, 0));
	OverlayImage(surface, parts.layerrightlegfront, cv::Point(0, 0));
	OverlayImage(surface, parts.layerleftlegfront, cv::Point(0, 0));
	OverlayImage(surface, parts.layerleftarmfront, cv::Point(0, 0));

	// Preserve the opaque contour while adding the official render's soft outer
	// coverage ramp. Union the blurred alpha with the original instead of
	// blurring the contour inward; skin texel boundaries remain crisp.
	cv::Mat alpha;
	cv::extractChannel(surface, alpha, 3);
	cv::Mat blurred_alpha;
	cv::GaussianBlur(alpha, blurred_alpha, cv::Size(0, 0), 1.9, 1.9, cv::BORDER_CONSTANT);
	cv::Mat alpha_float;
	cv::Mat blurred_float;
	alpha.convertTo(alpha_float, CV_32F, 1.0 / 255.0);
	blurred_alpha.convertTo(blurred_float, CV_32F, 1.0 / 255.0);
	cv::Mat union_alpha = alpha_float + blurred_float - alpha_float.mul(blurred_float);
	union_alpha *= 255.0;
	union_alpha.convertTo(alpha, CV_8U);
	cv::insertChannel(alpha, surface, 3);

	return surface;
}

} // namespace

cv::Mat CreateRender(cv::Mat& skin, bool model, const cv::Mat& cape) {
	const cv::Mat cape_layer = CreateCapeRenderLayer(cape);

	if (model)
	{
		cv::Mat HEAD_SHADOW = LoadRequiredPng("Resources/HEAD_SHADOW.png");
		cv::Mat LEG_SHADOW = LoadRequiredPng("Resources/LEG_SHADOW.png");

		cv::Mat headfront = LitFace(skin, cv::Rect(8, 8, 8, 8), CubeFace::HeadFront);
		cv::Mat headside = LitFace(skin, cv::Rect(0, 8, 8, 8), CubeFace::HeadSide);
		cv::Mat headbottom = LitFace(skin, cv::Rect(16, 0, 8, 8), CubeFace::HeadBottom);
		cv::Mat layerheadside = LitFace(skin, cv::Rect(32, 8, 8, 8), CubeFace::HeadSide);
		cv::Mat layerheadfront = LitFace(skin, cv::Rect(40, 8, 8, 8), CubeFace::HeadFront);

		// Slim arms are three texels wide but are still warped against the
		// four-texel arm quad, so their lighting has to be laid out over that quad.
		const cv::Size slim_arm_quad(400, 1200);
		cv::Mat rightarmfront = LitFace(skin, cv::Rect(44, 20, 3, 12), CubeFace::ArmRightFront, slim_arm_quad);
		cv::Mat leftarmfront = LitFace(skin, cv::Rect(36, 52, 3, 12), CubeFace::ArmLeftFront, slim_arm_quad);
		cv::Mat layerrightarmfront = LitFace(skin, cv::Rect(44, 36, 3, 12), CubeFace::ArmRightFront, slim_arm_quad);
		cv::Mat layerleftarmfront = LitFace(skin, cv::Rect(52, 52, 3, 12), CubeFace::ArmLeftFront, slim_arm_quad);

		cv::Mat rightarmside = LitFace(skin, cv::Rect(40, 20, 4, 12), CubeFace::ArmRightSide);
		cv::Mat leftarmside = LitFace(skin, cv::Rect(32, 52, 4, 12), CubeFace::ArmLeftSide);
		cv::Mat layerrightarmside = LitFace(skin, cv::Rect(40, 36, 4, 12), CubeFace::ArmRightSide);
		cv::Mat layerleftarmside = LitFace(skin, cv::Rect(48, 52, 4, 12), CubeFace::ArmLeftSide);

		cv::Mat bodyfront = LitFace(skin, cv::Rect(20, 20, 8, 12), CubeFace::BodyFront);
		cv::Mat bodyside = LitFace(skin, cv::Rect(16, 20, 4, 12), CubeFace::BodySide);
		cv::Mat layerbodyfront = LitFace(skin, cv::Rect(20, 36, 8, 12), CubeFace::BodyFront);
		cv::Mat layerbodyside = LitFace(skin, cv::Rect(16, 36, 4, 12), CubeFace::BodySide);

		cv::Mat rightlegside = LitFace(skin, cv::Rect(0, 20, 4, 12), CubeFace::LegRightSide);
		cv::Mat rightlegfront = LitFace(skin, cv::Rect(4, 20, 4, 12), CubeFace::LegRightFront);
		cv::Mat leftlegfront = LitFace(skin, cv::Rect(20, 52, 4, 12), CubeFace::LegLeftFront);
		cv::Mat layerrightlegside = LitFace(skin, cv::Rect(0, 36, 4, 12), CubeFace::LegRightSide);
		cv::Mat layerrightlegfront = LitFace(skin, cv::Rect(4, 36, 4, 12), CubeFace::LegRightFront);
		cv::Mat layerleftlegfront = LitFace(skin, cv::Rect(4, 52, 4, 12), CubeFace::LegLeftFront);

		RenderPerspectiveTransformation(168, 512, 376, 530, 345, 1200, 136, 1196, PartSize::Size4x12, rightarmfront);
		RenderPerspectiveTransformation(98, 521, 168, 512, 136, 1196, 68, 1175, PartSize::Size4x12, rightarmside);

		RenderPerspectiveTransformation(725, 532, 915, 525, 936, 1160, 749, 1177, PartSize::Size4x12, leftarmfront);
		RenderPerspectiveTransformation(627, 544, 725, 532, 749, 1177, 651, 1143, PartSize::Size4x12, leftarmside);

		RenderPerspectiveTransformation(158, 498, 339, 511, 310, 1215, 128, 1217, PartSize::Size4x12, layerrightarmfront);
		RenderPerspectiveTransformation(75, 509, 158, 498, 128, 1217, 49, 1183, PartSize::Size4x12, layerrightarmside);

		RenderPerspectiveTransformation(717, 521, 878, 514, 899, 1175, 742, 1196, PartSize::Size4x12, layerleftarmfront);
		RenderPerspectiveTransformation(611, 570, 717, 521, 742, 1196, 630, 1155, PartSize::Size4x12, layerleftarmside);

		RenderPerspectiveTransformation(366, 59, 776, 86, 774, 529, 368, 521, PartSize::Head, headfront);
		RenderPerspectiveTransformation(210, 119, 366, 59, 368, 521, 212, 537, PartSize::Head, headside);
		RenderPerspectiveTransformation(366, 520, 774, 529, 591, 537, 212, 537, PartSize::Head, headbottom);

		RenderPerspectiveTransformation(327, 1194, 528, 1184, 519, 1825, 320, 1846, PartSize::Size4x12, rightlegfront);
		RenderPerspectiveTransformation(249, 1161, 327, 1194, 320, 1846, 244, 1793, PartSize::Size4x12, rightlegside);
		RenderPerspectiveTransformation(528, 1184, 722, 1175, 720, 1801, 529, 1824, PartSize::Size4x12, leftlegfront);

		RenderPerspectiveTransformation(325, 526, 725, 532, 722, 1175, 326, 1194, PartSize::Body, bodyfront);
		RenderPerspectiveTransformation(252, 534, 325, 526, 326, 1196, 249, 1167, PartSize::Size4x12, bodyside);

		RenderPerspectiveTransformation(319, 1192, 542, 1173, 524, 1847, 311, 1864, PartSize::Size4x12, layerrightlegfront);
		RenderPerspectiveTransformation(239, 1183, 319, 1192, 311, 1864, 234, 1799, PartSize::Size4x12, layerrightlegside);
		RenderPerspectiveTransformation(514, 1175, 741, 1157, 740, 1820, 524, 1847, PartSize::Size4x12, layerleftlegfront);

		RenderPerspectiveTransformation(350, 23, 813, 56, 813, 548, 352, 547, PartSize::Head, layerheadfront);
		RenderPerspectiveTransformation(176, 94, 350, 23, 352, 547, 178, 564, PartSize::Head, layerheadside);

		RenderPerspectiveTransformation(318, 504, 743, 512, 742, 1175, 323, 1196, PartSize::Body, layerbodyfront);
		RenderPerspectiveTransformation(241, 515, 318, 504, 323, 1196, 244, 1166, PartSize::Size4x12, layerbodyside);

		return ComposeRenderedSurface(
			{
				headfront, headside, headbottom, layerheadside, layerheadfront,
				rightarmfront, leftarmfront, layerrightarmfront, layerleftarmfront,
				rightarmside, leftarmside, layerrightarmside, layerleftarmside,
				bodyfront, bodyside, layerbodyfront, layerbodyside,
				rightlegside, rightlegfront, leftlegfront, layerrightlegside, layerrightlegfront, layerleftlegfront
			},
			HEAD_SHADOW,
			LEG_SHADOW,
			cape_layer
		);
	}
	else
	{
		cv::Mat HEAD_SHADOW = LoadRequiredPng("Resources/HEAD_SHADOW.png");
		cv::Mat LEG_SHADOW = LoadRequiredPng("Resources/LEG_SHADOW.png");

		cv::Mat headfront = LitFace(skin, cv::Rect(8, 8, 8, 8), CubeFace::HeadFront);
		cv::Mat headside = LitFace(skin, cv::Rect(0, 8, 8, 8), CubeFace::HeadSide);
		cv::Mat headbottom = LitFace(skin, cv::Rect(16, 0, 8, 8), CubeFace::HeadBottom);
		cv::Mat layerheadside = LitFace(skin, cv::Rect(32, 8, 8, 8), CubeFace::HeadSide);
		cv::Mat layerheadfront = LitFace(skin, cv::Rect(40, 8, 8, 8), CubeFace::HeadFront);

		cv::Mat rightarmfront = LitFace(skin, cv::Rect(44, 20, 4, 12), CubeFace::ArmRightFront);
		cv::Mat leftarmfront = LitFace(skin, cv::Rect(36, 52, 4, 12), CubeFace::ArmLeftFront);
		cv::Mat layerrightarmfront = LitFace(skin, cv::Rect(44, 36, 4, 12), CubeFace::ArmRightFront);
		cv::Mat layerleftarmfront = LitFace(skin, cv::Rect(52, 52, 4, 12), CubeFace::ArmLeftFront);

		cv::Mat rightarmside = LitFace(skin, cv::Rect(40, 20, 4, 12), CubeFace::ArmRightSide);
		cv::Mat leftarmside = LitFace(skin, cv::Rect(32, 52, 4, 12), CubeFace::ArmLeftSide);
		cv::Mat layerrightarmside = LitFace(skin, cv::Rect(40, 36, 4, 12), CubeFace::ArmRightSide);
		cv::Mat layerleftarmside = LitFace(skin, cv::Rect(48, 52, 4, 12), CubeFace::ArmLeftSide);

		cv::Mat bodyfront = LitFace(skin, cv::Rect(20, 20, 8, 12), CubeFace::BodyFront);
		cv::Mat bodyside = LitFace(skin, cv::Rect(16, 20, 4, 12), CubeFace::BodySide);
		cv::Mat layerbodyfront = LitFace(skin, cv::Rect(20, 36, 8, 12), CubeFace::BodyFront);
		cv::Mat layerbodyside = LitFace(skin, cv::Rect(16, 36, 4, 12), CubeFace::BodySide);

		cv::Mat rightlegside = LitFace(skin, cv::Rect(0, 20, 4, 12), CubeFace::LegRightSide);
		cv::Mat rightlegfront = LitFace(skin, cv::Rect(4, 20, 4, 12), CubeFace::LegRightFront);
		cv::Mat leftlegfront = LitFace(skin, cv::Rect(20, 52, 4, 12), CubeFace::LegLeftFront);
		cv::Mat layerrightlegside = LitFace(skin, cv::Rect(0, 36, 4, 12), CubeFace::LegRightSide);
		cv::Mat layerrightlegfront = LitFace(skin, cv::Rect(4, 36, 4, 12), CubeFace::LegRightFront);
		cv::Mat layerleftlegfront = LitFace(skin, cv::Rect(4, 52, 4, 12), CubeFace::LegLeftFront);

		RenderPerspectiveTransformation(120, 512, 328, 526, 305, 1194, 94, 1194, PartSize::Size4x12, rightarmfront);
		RenderPerspectiveTransformation(51, 522, 120, 512, 94, 1194, 26, 1172, PartSize::Size4x12, rightarmside);

		RenderPerspectiveTransformation(716, 532, 902, 527, 924, 1162, 740, 1170, PartSize::Size4x12, leftarmfront);
		RenderPerspectiveTransformation(627, 534, 715, 500, 740, 1171, 651, 1143, PartSize::Size4x12, leftarmside);

		RenderPerspectiveTransformation(119, 496, 346, 513, 325, 1211, 92, 1213, PartSize::Size4x12, layerrightarmfront);
		RenderPerspectiveTransformation(34, 506, 119, 496, 92, 1213, 9, 1183, PartSize::Size4x12, layerrightarmside);

		RenderPerspectiveTransformation(709, 523, 919, 510, 944, 1163, 735, 1187, PartSize::Size4x12, layerleftarmfront);
		RenderPerspectiveTransformation(611, 563, 709, 523, 735, 1187, 635, 1165, PartSize::Size4x12, layerleftarmside);

		RenderPerspectiveTransformation(366, 59, 776, 86, 774, 529, 368, 521, PartSize::Head, headfront);
		RenderPerspectiveTransformation(210, 119, 366, 59, 368, 521, 212, 537, PartSize::Head, headside);
		RenderPerspectiveTransformation(366, 520, 774, 529, 591, 537, 212, 537, PartSize::Head, headbottom);

		RenderPerspectiveTransformation(327, 1194, 528, 1184, 519, 1825, 320, 1846, PartSize::Size4x12, rightlegfront);
		RenderPerspectiveTransformation(249, 1161, 327, 1194, 320, 1846, 244, 1793, PartSize::Size4x12, rightlegside);
		RenderPerspectiveTransformation(528, 1184, 722, 1175, 720, 1801, 529, 1824, PartSize::Size4x12, leftlegfront);

		RenderPerspectiveTransformation(325, 526, 725, 532, 722, 1175, 326, 1194, PartSize::Body, bodyfront);
		RenderPerspectiveTransformation(252, 534, 325, 526, 326, 1196, 249, 1167, PartSize::Size4x12, bodyside);

		RenderPerspectiveTransformation(319, 1192, 542, 1173, 524, 1847, 311, 1864, PartSize::Size4x12, layerrightlegfront);
		RenderPerspectiveTransformation(239, 1183, 319, 1192, 311, 1864, 234, 1799, PartSize::Size4x12, layerrightlegside);
		RenderPerspectiveTransformation(514, 1175, 741, 1157, 740, 1820, 524, 1847, PartSize::Size4x12, layerleftlegfront);

		RenderPerspectiveTransformation(350, 23, 813, 56, 813, 548, 352, 547, PartSize::Head, layerheadfront);
		RenderPerspectiveTransformation(176, 94, 350, 23, 352, 547, 178, 564, PartSize::Head, layerheadside);

		RenderPerspectiveTransformation(318, 504, 743, 512, 742, 1175, 323, 1196, PartSize::Body, layerbodyfront);
		RenderPerspectiveTransformation(241, 515, 318, 504, 323, 1196, 244, 1166, PartSize::Size4x12, layerbodyside);

		return ComposeRenderedSurface(
			{
				headfront, headside, headbottom, layerheadside, layerheadfront,
				rightarmfront, leftarmfront, layerrightarmfront, layerleftarmfront,
				rightarmside, leftarmside, layerrightarmside, layerleftarmside,
				bodyfront, bodyside, layerbodyfront, layerbodyside,
				rightlegside, rightlegfront, leftlegfront, layerrightlegside, layerrightlegfront, layerleftlegfront
			},
			HEAD_SHADOW,
			LEG_SHADOW,
			cape_layer
		);
	}
}

int main(int argc, char* argv[]) {

	auto print_usage = []() {
		std::cout << "Usage:" << std::endl;
		std::cout << "  SteveModMaker <minecraft_username> [options] <slot_number> [arm_type]" << std::endl;
		std::cout << "  SteveModMaker --skin-file <skin_png_path> [options] <slot_number> [arm_type]" << std::endl;
		std::cout << "  slot_number: Costume slot (1-8)" << std::endl;
		std::cout << "  arm_type (optional): 'small' or 'big' to override auto-detection" << std::endl;
		std::cout << "  --cape: use the Minecraft cape attached to a username" << std::endl;
		std::cout << "  --cape-official <id>: use an official cape from --list-capes" << std::endl;
		std::cout << "  --cape-file <cape_png_path>: use a local 64x32 Minecraft cape atlas" << std::endl;
		std::cout << "  --render-png <png_path>: write the CSS character-select render as a PNG" << std::endl;
		std::cout << "  --list-capes: list embedded official cape IDs" << std::endl;
	};

	if (argc == 2 && std::string(argv[1]) == "--list-capes") {
		for (const CapeCatalogEntry& cape : OfficialCapeCatalog()) {
			std::cout << cape.id << '\t' << cape.name << std::endl;
		}
		return 0;
	}

	if (argc < 3) {
		print_usage();
		return -1;
	}

	try {

		bool use_skin_file = false;
		std::string skin_source;
		std::string player_name;
		std::string patch_subdir;
		std::string special_message;
		std::string cape_file;
		std::string official_cape_id;
		std::string render_png;
		std::string forced_arm;
		bool cape_enabled = false;
		bool account_cape_requested = false;
		bool cape_option_seen = false;
		bool player_name_option_seen = false;
		int slot_input = 0;

		const std::string first_arg = argv[1];
		if (first_arg == "--skin-file" || first_arg == "-f") {
			if (argc < 4) {
				print_usage();
				return -1;
			}
			use_skin_file = true;
			skin_source = argv[2];

			int idx = 3;
			while (idx < argc) {
				const std::string option = argv[idx];
				if (option == "--cape") {
					if (cape_option_seen) {
						std::cerr << "Error: Choose only one cape source: --cape, --cape-official, or --cape-file" << std::endl;
						return -1;
					}
					cape_option_seen = true;
					cape_enabled = true;
					account_cape_requested = true;
					++idx;
					continue;
				}
				if (option == "--cape-official") {
					if (cape_option_seen) {
						std::cerr << "Error: Choose only one cape source: --cape, --cape-official, or --cape-file" << std::endl;
						return -1;
					}
					if (idx + 1 >= argc) {
						std::cout << "Error: --cape-official requires a value" << std::endl;
						return -1;
					}
					cape_option_seen = true;
					official_cape_id = argv[idx + 1];
					cape_enabled = true;
					idx += 2;
					continue;
				}
				if (option == "--cape-file") {
					if (cape_option_seen) {
						std::cerr << "Error: Choose only one cape source: --cape, --cape-official, or --cape-file" << std::endl;
						return -1;
					}
					if (idx + 1 >= argc) {
						std::cout << "Error: --cape-file requires a value" << std::endl;
						return -1;
					}
					cape_option_seen = true;
					cape_file = argv[idx + 1];
					cape_enabled = true;
					idx += 2;
					continue;
				}
				if (option == "--player-name" || option == "-n") {
					if (idx + 1 >= argc) {
						std::cout << "Error: --player-name requires a value" << std::endl;
						return -1;
					}
					player_name_option_seen = true;
					player_name = argv[idx + 1];
					idx += 2;
					continue;
				}
				if (option == "--patch-subdir" || option == "-p") {
					if (idx + 1 >= argc) {
						std::cout << "Error: --patch-subdir requires a value" << std::endl;
						return -1;
					}
					patch_subdir = argv[idx + 1];
					idx += 2;
					continue;
				}
				if (option == "--special-message" || option == "-m") {
					if (idx + 1 >= argc) {
						std::cout << "Error: --special-message requires a value" << std::endl;
						return -1;
					}
					special_message = argv[idx + 1];
					idx += 2;
					continue;
				}
				if (option == "--render-png") {
					if (idx + 1 >= argc) {
						std::cout << "Error: --render-png requires a value" << std::endl;
						return -1;
					}
					render_png = argv[idx + 1];
					idx += 2;
					continue;
				}
				break;
			}

			if (idx >= argc) {
				print_usage();
				return -1;
			}
			if (!TryParseSlotNumber(argv[idx], slot_input)) {
				std::cerr << "Error: Slot number must be an integer between 1 and 8" << std::endl;
				return -1;
			}
			++idx;

			if (idx < argc) {
				forced_arm = argv[idx];
				++idx;
			}

			if (idx != argc) {
				print_usage();
				return -1;
			}
		}
		else {
			skin_source = argv[1];
			int idx = 2;
			while (idx < argc) {
				const std::string option = argv[idx];
				if (option == "--cape") {
					if (cape_option_seen) {
						std::cerr << "Error: Choose only one cape source: --cape, --cape-official, or --cape-file" << std::endl;
						return -1;
					}
					cape_option_seen = true;
					cape_enabled = true;
					account_cape_requested = true;
					++idx;
					continue;
				}
				if (option == "--cape-official") {
					if (cape_option_seen) {
						std::cerr << "Error: Choose only one cape source: --cape, --cape-official, or --cape-file" << std::endl;
						return -1;
					}
					if (idx + 1 >= argc) {
						std::cout << "Error: --cape-official requires a value" << std::endl;
						return -1;
					}
					cape_option_seen = true;
					official_cape_id = argv[idx + 1];
					cape_enabled = true;
					idx += 2;
					continue;
				}
				if (option == "--cape-file") {
					if (cape_option_seen) {
						std::cerr << "Error: Choose only one cape source: --cape, --cape-official, or --cape-file" << std::endl;
						return -1;
					}
					if (idx + 1 >= argc) {
						std::cout << "Error: --cape-file requires a value" << std::endl;
						return -1;
					}
					cape_option_seen = true;
					cape_file = argv[idx + 1];
					cape_enabled = true;
					idx += 2;
					continue;
				}
				if (option == "--patch-subdir" || option == "-p") {
					if (idx + 1 >= argc) {
						std::cout << "Error: --patch-subdir requires a value" << std::endl;
						return -1;
					}
					patch_subdir = argv[idx + 1];
					idx += 2;
					continue;
				}
				if (option == "--special-message" || option == "-m") {
					if (idx + 1 >= argc) {
						std::cout << "Error: --special-message requires a value" << std::endl;
						return -1;
					}
					special_message = argv[idx + 1];
					idx += 2;
					continue;
				}
				if (option == "--render-png") {
					if (idx + 1 >= argc) {
						std::cout << "Error: --render-png requires a value" << std::endl;
						return -1;
					}
					render_png = argv[idx + 1];
					idx += 2;
					continue;
				}
				break;
			}

			if (idx >= argc) {
				print_usage();
				return -1;
			}
			if (!TryParseSlotNumber(argv[idx], slot_input)) {
				std::cerr << "Error: Slot number must be an integer between 1 and 8" << std::endl;
				return -1;
			}
			++idx;

			if (idx < argc) {
				forced_arm = argv[idx];
				++idx;
			}
			if (idx != argc) {
				print_usage();
				return -1;
			}
		}

		const int cape_source_count = static_cast<int>(account_cape_requested)
			+ static_cast<int>(!cape_file.empty())
			+ static_cast<int>(!official_cape_id.empty());
		if (cape_source_count > 1) {
			std::cerr << "Error: Choose only one cape source: --cape, --cape-official, or --cape-file" << std::endl;
			return -1;
		}
		if (cape_enabled && cape_source_count != 1) {
			std::cerr << "Error: A cape option requires a non-empty cape source value" << std::endl;
			return -1;
		}
		if (!use_skin_file && !IsValidMinecraftUsername(skin_source)) {
			std::cerr << "Error: Minecraft username must be 1-16 letters, numbers, or underscores" << std::endl;
			return -1;
		}
		if (!patch_subdir.empty() && !IsSafeRelativeOutputPath(patch_subdir)) {
			std::cerr << "Error: --patch-subdir must be a safe relative path without '.' or '..' components" << std::endl;
			return -1;
		}

		// Parse slot number (1-8)
		if (slot_input < 1 || slot_input > 8) {
			std::cout << "Error: Slot number must be between 1 and 8" << std::endl;
			return -1;
		}

		uint8_t C0X_ = slot_input - 1; // Convert 1-8 to 0-7

		// Format as two-digit string (00-07)
		char buffer[3];
		snprintf(buffer, sizeof(buffer), "%02d", C0X_);
		std::string C0X(buffer);
		std::filesystem::path output_root = ".";
		if (!patch_subdir.empty()) {
			output_root /= patch_subdir;
		}

		cv::Mat skin;
		if (use_skin_file) {
			std::cout << "[SteveModMaker::Main] Loading skin file: " << skin_source << std::endl;
			skin = LoadSkinFromFile(skin_source);
		}
		else {
			std::cout << "[SteveModMaker::Main] Downloading skin for " << skin_source << "..." << std::endl;
			skin = DownloadSkin(skin_source);
		}
		if (skin.empty()) {
			std::cerr << "[SteveModMaker::Main] Failed to load skin input" << std::endl;
			return -1;
		}

		cv::Mat cape;
		if (cape_enabled) {
			if (!cape_file.empty()) {
				std::cout << "[SteveModMaker::Main] Loading Minecraft cape file: " << cape_file << std::endl;
				cape = LoadCapeFromFile(cape_file);
			}
			else if (!official_cape_id.empty()) {
				const CapeCatalogEntry* official_cape = FindOfficialCape(official_cape_id);
				if (official_cape == nullptr) {
					std::cerr << "[SteveModMaker::Main] Error: Unknown official cape ID: " << official_cape_id
						<< ". Run --list-capes to see valid IDs." << std::endl;
					return -1;
				}
				std::cout << "[SteveModMaker::Main] Loading official Minecraft cape: " << official_cape->name << std::endl;
				cape = LoadOfficialCape(official_cape_id);
			}
			else if (use_skin_file) {
				std::cerr << "[SteveModMaker::Main] Error: account cape mode requires a Minecraft username" << std::endl;
				return -1;
			}
			else {
				std::cout << "[SteveModMaker::Main] Downloading Minecraft cape for " << skin_source << "..." << std::endl;
				cape = DownloadCape(skin_source);
			}
			if (cape.empty()) {
				std::cerr << "[SteveModMaker::Main] Failed to load cape input" << std::endl;
				return -1;
			}
			std::cout << "[SteveModMaker::Main] Minecraft cape ready at canonical 64x32 atlas size." << std::endl;
		}

		std::cout << "[SteveModMaker::Main] Determining player model..." << std::endl;
		bool model = DetectSlimModelFromSkin(skin);
		std::string arm_type = model ? "small" : "big";

		if (!forced_arm.empty()) {
			if (forced_arm == "small") {
				model = true;
				arm_type = "small";
				std::cout << "[SteveModMaker::Main] Using manual arm override: small" << std::endl;
			}
			else if (forced_arm == "big") {
				model = false;
				arm_type = "big";
				std::cout << "[SteveModMaker::Main] Using manual arm override: big" << std::endl;
			}
			else {
				std::cout << "Error: arm_type must be 'small' or 'big'" << std::endl;
				return -1;
			}
		}
		else {
			std::cout << "[SteveModMaker::Main] Auto-detected " << arm_type << " arms template from skin texture" << std::endl;
		}

		if (player_name.empty() && !player_name_option_seen) {
			player_name = use_skin_file ? std::filesystem::path(skin_source).stem().string() : skin_source;
		}
		if (player_name.empty() || player_name.find_first_not_of(' ') == std::string::npos) {
			std::cerr << "[SteveModMaker::Main] Error: Player name cannot be empty" << std::endl;
			return -1;
		}
		if (special_message.empty()) {
			special_message = player_name;
		}
		if (!IsAsciiText(player_name)) {
			std::cerr << "[SteveModMaker::Main] Error: Player name must contain printable ASCII characters only" << std::endl;
			return -1;
		}
		if (!IsAsciiText(special_message)) {
			std::cerr << "[SteveModMaker::Main] Error: Special message must contain printable ASCII characters only" << std::endl;
			return -1;
		}
	
	skin = ConvertToModernSkin(skin, model);

	std::cout << "[SteveModMaker::Main] Skin ready." << std::endl;

		cv::Mat base_render = CreateRender(skin, model, cape);

		std::cout << "[SteveModMaker::Main] Created Render." << std::endl;
		if (!render_png.empty()) {
			if (!WriteRenderPng(base_render, render_png)) {
				std::cerr << "[SteveModMaker::Main] Error: Failed to write PNG render: " << render_png << std::endl;
				return -1;
			}
			std::cout << "[SteveModMaker::Main] Wrote CSS render PNG: " << render_png << std::endl;
		}

#ifdef _DEBUG
		if (!imwrite("Final_Render.png", base_render)) {
			std::cerr << "[SteveModMaker::Main] Warning: Failed to write debug render PNG" << std::endl;
		}
#endif

	std::cout << "[SteveModMaker::Main] Creating output directories..." << std::endl;

		const std::filesystem::path target_slot = output_root / "fighter/pickel/model/body" / ("c" + C0X);
	std::cout << "[SteveModMaker::Main] Writing fighter template files..." << std::endl;
	std::filesystem::create_directories(target_slot);
		const std::array<const char*, 9> fighter_template_files = {
		"dark_model.numatb",
		"light_model.numatb",
		"metamon_model.numatb",
		"model.numatb",
		"model.numdlb",
		"model.numshb",
		"model.numshexb",
		"model.nusrcmdlb",
		"model.xmb"
	};
	for (const char* file_name : fighter_template_files) {
		if (!WriteRequiredTemplate(arm_type, cape_enabled, file_name, target_slot / file_name)) {
			std::cerr << "[SteveModMaker::Main] Error: Missing embedded fighter template file: " << file_name << std::endl;
			return -1;
		}
	}
		if (cape_enabled) {
			for (const char* file_name : {"model.nusktb", "cape.nutexb"}) {
				if (!WriteRequiredTemplate(arm_type, true, file_name, target_slot / file_name)) {
					std::cerr << "[SteveModMaker::Main] Error: Missing embedded cape fighter template file: " << file_name << std::endl;
					return -1;
				}
			}
			const std::filesystem::path motion_body = output_root / "fighter/pickel/motion/body" / ("c" + C0X);
			std::filesystem::create_directories(motion_body);
			for (const char* file_name : {
				"swing.prc",
				"update.prc",
				"a00defaulteyelid.nuanmb",
				"d02specialhistart.nuanmb",
				"d02specialairhistart.nuanmb",
				"d02specialairhi.nuanmb",
				"d02specialairhimax.nuanmb"
			}) {
				if (!WriteRequiredTemplate(arm_type, true, file_name, motion_body / file_name)) {
					std::cerr << "[SteveModMaker::Main] Error: Missing embedded cape motion template file: " << file_name << std::endl;
					return -1;
				}
			}
			if (!WriteCapeArcropolisConfig(C0X, output_root)) {
				std::cerr << "[SteveModMaker::Main] Error: Failed to write ARCropolis cape file registration" << std::endl;
				return -1;
			}
			std::cout << "[SteveModMaker::Main] Using separate Minecraft cape model template with swing physics." << std::endl;
		}
	
		// Ensure UI directories exist for character select screen files.
		// These are UI part buckets, not skin-slot buckets.
		std::filesystem::create_directories(output_root / "ui/replace_patch/chara/chara_0");
		std::filesystem::create_directories(output_root / "ui/replace_patch/chara/chara_1");
		std::filesystem::create_directories(output_root / "ui/replace_patch/chara/chara_2");
		std::filesystem::create_directories(output_root / "ui/replace_patch/chara/chara_3");
		std::filesystem::create_directories(output_root / "ui/replace_patch/chara/chara_4");
		std::filesystem::create_directories(output_root / "ui/replace_patch/chara/chara_6");
		std::filesystem::remove_all(output_root / "ui/replace_patch/chara/chara_5");
		std::cout << "[SteveModMaker::Main] Writing ui_chara_db.prcx..." << std::endl;
		if (!WriteSlotUiCharaDb(C0X, output_root)) {
			std::cerr
				<< "[SteveModMaker::Main] Error: Missing bundled ui_chara_db template for slot code "
				<< C0X
				<< std::endl;
			return -1;
		}
		const std::filesystem::path message_file_path = output_root / "ui/message/msg_name.xmsbt";
		std::cout << "[SteveModMaker::Main] Writing message name file..." << std::endl;
		if (!WriteMsgNameTemplate(player_name, special_message, slot_input == 1, MsgNameSlotCode(slot_input), message_file_path)) {
			std::cerr << "[SteveModMaker::Main] Error: Failed to write ui/message/msg_name.xmsbt template output" << std::endl;
			return -1;
		}

		cv::Mat chara_0;
		cv::Mat chara_1;
		cv::Mat chara_4;

	cv::Mat chara_2 = LoadRequiredPng("Chara_Masks/chara_2_pickel_00.png");
	cv::Mat chara_4_mask = LoadRequiredPng("Chara_Masks/chara_4_mask.png");

	if (C0X_ % 2 == 0) { // Uses a steve slot.
		chara_0 = LoadRequiredPng("Chara_Masks/chara_0_pickel_00.png");
		chara_4 = LoadRequiredPng("Chara_Masks/chara_4_pickel_00.png");
	}
	else {
		chara_0 = LoadRequiredPng("Chara_Masks/chara_0_pickel_01.png");
		chara_4 = LoadRequiredPng("Chara_Masks/chara_4_pickel_01.png");
	}

	// chara_1 shadow/mask should match selected arm size, not costume slot.
	chara_1 = model
		? LoadRequiredPng("Chara_Masks/chara_1_pickel_01.png")
		: LoadRequiredPng("Chara_Masks/chara_1_pickel_00.png");

	std::cout << "[SteveModMaker::Main] Creating chara_0 image..." << std::endl;
	{ // chara_0
		cv::Mat render_cpy(176, 336, CV_8UC4);
		cv::resize(base_render, render_cpy, cv::Size(176, 336), 0, 0, cv::INTER_LANCZOS4);
			OverlayImage(chara_0, render_cpy, cv::Point(-25, -2));

			BNTX bntx(chara_0, "chara_0_pickel_" + C0X);
			if (!bntx.Write((output_root / "ui/replace_patch/chara/chara_0" / ("chara_0_pickel_" + C0X + ".bntx")).string())) {
				std::cerr << "[SteveModMaker::Main] Error: Failed to write chara_0 BNTX" << std::endl;
				return -1;
			}
		}
	std::cout << "[SteveModMaker::Main] Creating chara_1 image..." << std::endl;
	{ // chara_1
		cv::Mat render_cpy(280, 540, CV_8UC4);
			cv::resize(base_render, render_cpy, cv::Size(280, 540), 0, 0, cv::INTER_LANCZOS4);
			OverlayImage(chara_1, render_cpy, cv::Point(93, 63));
			BNTX bntx(chara_1, "chara_1_pickel_" + C0X);
			if (!bntx.Write((output_root / "ui/replace_patch/chara/chara_1" / ("chara_1_pickel_" + C0X + ".bntx")).string())) {
				std::cerr << "[SteveModMaker::Main] Error: Failed to write chara_1 BNTX" << std::endl;
				return -1;
			}
		}
	std::cout << "[SteveModMaker::Main] Creating chara_2 image..." << std::endl;
	{ // chara_2
		cv::Mat head = skin(cv::Rect(8, 8, 8, 8)).clone();
		const cv::Mat head_layer = skin(cv::Rect(40, 8, 8, 8));
		OverlayImage(head, head_layer, cv::Point(0, 0));
		cv::Mat head_scaled(45, 45, CV_8UC4);
			cv::resize(head, head_scaled, cv::Size(45, 45), 0, 0, cv::INTER_NEAREST);
			OverlayImage(chara_2, head_scaled, cv::Point(10, 9));
			BNTX bntx(chara_2, "chara_2_pickel_" + C0X);
			if (!bntx.Write((output_root / "ui/replace_patch/chara/chara_2" / ("chara_2_pickel_" + C0X + ".bntx")).string())) {
				std::cerr << "[SteveModMaker::Main] Error: Failed to write chara_2 BNTX" << std::endl;
				return -1;
			}
		}
		std::cout << "[SteveModMaker::Main] Creating chara_3 image..." << std::endl;
		{ // chara_3
			BNTX bntx(base_render, "chara_3_pickel_" + C0X);
			if (!bntx.Write((output_root / "ui/replace_patch/chara/chara_3" / ("chara_3_pickel_" + C0X + ".bntx")).string())) {
				std::cerr << "[SteveModMaker::Main] Error: Failed to write chara_3 BNTX" << std::endl;
				return -1;
			}
		}
	std::cout << "[SteveModMaker::Main] Creating chara_4 image..." << std::endl;
	{ // chara_4
		cv::Mat render_cpy(174, 333, CV_8UC4);
		cv::resize(base_render, render_cpy, cv::Size(174, 333), 0, 0, cv::INTER_LANCZOS4);
		OverlayImage(chara_4, render_cpy, cv::Point(-7, 8));
			Chara4Mask(chara_4, chara_4_mask);

			BNTX bntx(chara_4, "chara_4_pickel_" + C0X);
			if (!bntx.Write((output_root / "ui/replace_patch/chara/chara_4" / ("chara_4_pickel_" + C0X + ".bntx")).string())) {
				std::cerr << "[SteveModMaker::Main] Error: Failed to write chara_4 BNTX" << std::endl;
				return -1;
			}
		}
		std::cout << "[SteveModMaker::Main] Creating chara_6 image..." << std::endl;
		{ // chara_6
		cv::Mat chara_6(256, 512, CV_8UC4); std::memset(chara_6.data, 0, 256 * 512 * 4);

		cv::Mat render_cpy(711, 1385, CV_8UC4);
			cv::resize(base_render, render_cpy, cv::Size(711, 1385), 0, 0, cv::INTER_LANCZOS4);
			OverlayImage(chara_6, render_cpy, cv::Point(-150, -110));

		 	BNTX bntx(chara_6, "chara_6_pickel_" + C0X);
			if (!bntx.Write((output_root / "ui/replace_patch/chara/chara_6" / ("chara_6_pickel_" + C0X + ".bntx")).string())) {
				std::cerr << "[SteveModMaker::Main] Error: Failed to write chara_6 BNTX" << std::endl;
				return -1;
			}
		}
		std::cout << "[SteveModMaker::Main] Writing fighter texture..." << std::endl;
		ColorCorrectModelTexture(skin);
		const std::filesystem::path output_nutexb = output_root / "fighter/pickel/model/body" / ("c" + C0X) / "def_pickel_001_col.nutexb";
		// Cape-enabled Blender templates contain a BC7 placeholder. Replacing its
		// payload with raw pixels while retaining the BC7 footer corrupts Steve's
		// skin, so always create a correctly described uncompressed texture.
		NUTEXB generated_skin("def_pickel_001_col", skin, NUTEXBFormat::R8G8B8A8_SRGB);
		if (!generated_skin.Save(output_nutexb, 0)) {
			std::cerr << "[SteveModMaker::Main] Error: Failed to write fighter texture" << std::endl;
			return -1;
		}
		std::cout << "[SteveModMaker::Main] Wrote format-safe fighter texture (" << arm_type << " arms)" << std::endl;

		if (cape_enabled) {
			cv::Mat model_cape = cape.clone();
			ColorCorrectModelTexture(model_cape);
			const std::filesystem::path output_cape = output_root / "fighter/pickel/model/body" / ("c" + C0X) / "cape.nutexb";
			// The Blender exporter stores template textures as BC7. Replacing their
			// payload with raw pixels while retaining a BC7 footer corrupts the image,
			// so create a correctly described uncompressed texture for each cape.
			NUTEXB generated_cape("cape", model_cape, NUTEXBFormat::R8G8B8A8_SRGB);
			if (!generated_cape.Save(output_cape, 0)) {
				std::cerr << "[SteveModMaker::Main] Error: Failed to write cape texture" << std::endl;
				return -1;
			}
			std::cout << "[SteveModMaker::Main] Wrote swappable Minecraft cape texture: " << output_cape << std::endl;

			const std::filesystem::path output_wing = output_root / "fighter/pickel/model/wing" / ("c" + C0X) / "def_wing_001_col.nutexb";
			std::filesystem::create_directories(output_wing.parent_path());
			NUTEXB generated_wing("def_wing_001_col", model_cape, NUTEXBFormat::R8G8B8A8_SRGB);
			if (!generated_wing.Save(output_wing, 0)) {
				std::cerr << "[SteveModMaker::Main] Error: Failed to write Elytra texture" << std::endl;
				return -1;
			}
			std::cout << "[SteveModMaker::Main] Applied the selected cape atlas to Steve's Elytra: " << output_wing << std::endl;
		}

	std::cout << "[SteveModMaker::Main] Done!" << std::endl;

	return 0;

	}
	catch (const std::exception& ex) {
		std::cerr << "[SteveModMaker::Main] Fatal error: " << ex.what() << std::endl;
		return -1;
	}
	
}
