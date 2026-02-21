#include <iostream>
#include <array>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "Nutexb.hpp"
#include "BNTX.hpp"
#include "Constants.hpp"
#include "EmbeddedAssets.hpp"
#include "ImageUtils.hpp"
#include "MinecraftSkinUtil.hpp"

namespace {

cv::Mat LoadRequiredPng(const std::string& key) {
	cv::Mat img = EmbeddedAssets::LoadPng(key);
	if (img.empty()) {
		throw std::runtime_error("Missing embedded image asset: " + key);
	}
	return img;
}

bool WriteRequiredTemplate(const std::string& arm_type, const std::string& file_name, const std::filesystem::path& output_path) {
	const std::string template_dir = (arm_type == "small") ? "Templates/small_arms/" : "Templates/big_arms/";
	return EmbeddedAssets::WriteFile(template_dir + file_name, output_path);
}

} // namespace

cv::Mat CreateRender(cv::Mat& skin, bool model) {

	if (model)
	{
		cv::Mat HEAD_SHADOW = LoadRequiredPng("Resources/HEAD_SHADOW.png");
		cv::Mat LEG_SHADOW = LoadRequiredPng("Resources/LEG_SHADOW.png");
		cv::Mat LIGHTING = LoadRequiredPng("Resources/LIGHTING_EXP.png");

		cv::Mat headfront = CropAndScale(skin, cv::Rect(8, 8, 8, 8));
		cv::Mat headside = CropAndScale(skin, cv::Rect(0, 8, 8, 8));
		cv::Mat headbottom = CropAndScale(skin, cv::Rect(16, 0, 8, 8));
		cv::Mat layerheadside = CropAndScale(skin, cv::Rect(32, 8, 8, 8));
		cv::Mat layerheadfront = CropAndScale(skin, cv::Rect(40, 8, 8, 8));

		cv::Mat rightarmfront = CropAndScale(skin, cv::Rect(44, 20, 3, 12));
		cv::Mat leftarmfront = CropAndScale(skin, cv::Rect(36, 52, 3, 12));
		cv::Mat layerrightarmfront = CropAndScale(skin, cv::Rect(44, 36, 3, 12));
		cv::Mat layerleftarmfront = CropAndScale(skin, cv::Rect(52, 52, 3, 12));

		cv::Mat rightarmside = CropAndScale(skin, cv::Rect(40, 20, 4, 12));
		cv::Mat leftarmside = CropAndScale(skin, cv::Rect(32, 52, 4, 12));
		cv::Mat layerrightarmside = CropAndScale(skin, cv::Rect(40, 36, 4, 12));
		cv::Mat layerleftarmside = CropAndScale(skin, cv::Rect(48, 52, 4, 12));

		cv::Mat bodyfront = CropAndScale(skin, cv::Rect(20, 20, 8, 12));
		cv::Mat bodyside = CropAndScale(skin, cv::Rect(16, 20, 4, 12));
		cv::Mat layerbodyfront = CropAndScale(skin, cv::Rect(20, 36, 8, 12));
		cv::Mat layerbodyside = CropAndScale(skin, cv::Rect(16, 36, 4, 12));

		cv::Mat rightlegside = CropAndScale(skin, cv::Rect(0, 20, 4, 12));
		cv::Mat rightlegfront = CropAndScale(skin, cv::Rect(4, 20, 4, 12));
		cv::Mat leftlegfront = CropAndScale(skin, cv::Rect(20, 52, 4, 12));
		cv::Mat layerrightlegside = CropAndScale(skin, cv::Rect(0, 36, 4, 12));
		cv::Mat layerrightlegfront = CropAndScale(skin, cv::Rect(4, 36, 4, 12));
		cv::Mat layerleftlegfront = CropAndScale(skin, cv::Rect(4, 52, 4, 12));

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

		cv::Mat SURFACE(1864, 968, CV_8UC4);
		SURFACE.setTo(0);

		AdjustBrightness(bodyside, 0.2);
		AdjustBrightness(leftarmside, 0.3);

		AdjustBrightness(rightarmside, 0.6);
		AdjustBrightness(rightlegside, 0.4);
		AdjustBrightness(headside, 0.6);

		AdjustBrightness(layerbodyside, 0.2);
		AdjustBrightness(layerleftarmside, 0.3);

		AdjustBrightness(layerrightarmside, 0.6);
		AdjustBrightness(layerrightlegside, 0.4);
		AdjustBrightness(layerheadside, 0.6);

		OverlayImage(SURFACE, leftarmside, cv::Point(0, 0));

		OverlayImage(SURFACE, headbottom, cv::Point(0, 0));
		OverlayImage(SURFACE, bodyside, cv::Point(0, 0));

		OverlayImage(SURFACE, layerleftarmside, cv::Point(0, 0));

		OverlayImage(SURFACE, bodyfront, cv::Point(0, 0));

		OverlayImage(SURFACE, HEAD_SHADOW, cv::Point(0, 0));

		OverlayImage(SURFACE, layerbodyfront, cv::Point(0, 0));
		OverlayImage(SURFACE, layerbodyside, cv::Point(0, 0));

		OverlayImage(SURFACE, headfront, cv::Point(0, 0));
		OverlayImage(SURFACE, headside, cv::Point(0, 0));

		OverlayImage(SURFACE, rightlegside, cv::Point(0, 0));
		OverlayImage(SURFACE, rightlegfront, cv::Point(0, 0));
		OverlayImage(SURFACE, rightarmside, cv::Point(0, 0));
		OverlayImage(SURFACE, rightarmfront, cv::Point(0, 0));
		OverlayImage(SURFACE, leftlegfront, cv::Point(0, 0));
		OverlayImage(SURFACE, leftarmfront, cv::Point(0, 0));

		OverlayImage(SURFACE, layerheadfront, cv::Point(0, 0));
		OverlayImage(SURFACE, layerheadside, cv::Point(0, 0));

		OverlayImage(SURFACE, LEG_SHADOW, cv::Point(0, 0));

		OverlayImage(SURFACE, layerrightlegside, cv::Point(0, 0));
		OverlayImage(SURFACE, layerrightarmfront, cv::Point(0, 0));
		OverlayImage(SURFACE, layerrightarmside, cv::Point(0, 0));
		OverlayImage(SURFACE, layerrightlegfront, cv::Point(0, 0));
		OverlayImage(SURFACE, layerleftlegfront, cv::Point(0, 0));
		OverlayImage(SURFACE, layerleftarmfront, cv::Point(0, 0));

		std::vector<cv::Mat> surface_channels;
		cv::split(SURFACE, surface_channels);

		cv::Mat OUT_LIGHTING;

		LIGHTING.copyTo(OUT_LIGHTING, surface_channels[3]);

		OverlayImage(SURFACE, OUT_LIGHTING, cv::Point(0, 0));

		return SURFACE;
	}
	else
	{
		cv::Mat HEAD_SHADOW = LoadRequiredPng("Resources/HEAD_SHADOW.png");
		cv::Mat LEG_SHADOW = LoadRequiredPng("Resources/LEG_SHADOW.png");
		cv::Mat LIGHTING = LoadRequiredPng("Resources/LIGHTING_EXP.png");

		cv::Mat headfront = CropAndScale(skin, cv::Rect(8, 8, 8, 8));
		cv::Mat headside = CropAndScale(skin, cv::Rect(0, 8, 8, 8));
		cv::Mat headbottom = CropAndScale(skin, cv::Rect(16, 0, 8, 8));
		cv::Mat layerheadside = CropAndScale(skin, cv::Rect(32, 8, 8, 8));
		cv::Mat layerheadfront = CropAndScale(skin, cv::Rect(40, 8, 8, 8));

		cv::Mat rightarmfront = CropAndScale(skin, cv::Rect(44, 20, 4, 12));
		cv::Mat leftarmfront = CropAndScale(skin, cv::Rect(36, 52, 4, 12));
		cv::Mat layerrightarmfront = CropAndScale(skin, cv::Rect(44, 36, 4, 12));
		cv::Mat layerleftarmfront = CropAndScale(skin, cv::Rect(52, 52, 4, 12));

		cv::Mat rightarmside = CropAndScale(skin, cv::Rect(40, 20, 4, 12));
		cv::Mat leftarmside = CropAndScale(skin, cv::Rect(32, 52, 4, 12));
		cv::Mat layerrightarmside = CropAndScale(skin, cv::Rect(40, 36, 4, 12));
		cv::Mat layerleftarmside = CropAndScale(skin, cv::Rect(48, 52, 4, 12));

		cv::Mat bodyfront = CropAndScale(skin, cv::Rect(20, 20, 8, 12));
		cv::Mat bodyside = CropAndScale(skin, cv::Rect(16, 20, 4, 12));
		cv::Mat layerbodyfront = CropAndScale(skin, cv::Rect(20, 36, 8, 12));
		cv::Mat layerbodyside = CropAndScale(skin, cv::Rect(16, 36, 4, 12));

		cv::Mat rightlegside = CropAndScale(skin, cv::Rect(0, 20, 4, 12));
		cv::Mat rightlegfront = CropAndScale(skin, cv::Rect(4, 20, 4, 12));
		cv::Mat leftlegfront = CropAndScale(skin, cv::Rect(20, 52, 4, 12));
		cv::Mat layerrightlegside = CropAndScale(skin, cv::Rect(0, 36, 4, 12));
		cv::Mat layerrightlegfront = CropAndScale(skin, cv::Rect(4, 36, 4, 12));
		cv::Mat layerleftlegfront = CropAndScale(skin, cv::Rect(4, 52, 4, 12));

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

		cv::Mat SURFACE(1864, 968, CV_8UC4);
		SURFACE.setTo(0);

		AdjustBrightness(bodyside, 0.2);
		AdjustBrightness(leftarmside, 0.3);

		AdjustBrightness(rightarmside, 0.6);
		AdjustBrightness(rightlegside, 0.4);
		AdjustBrightness(headside, 0.6);

		AdjustBrightness(layerbodyside, 0.2);
		AdjustBrightness(layerleftarmside, 0.3);

		AdjustBrightness(layerrightarmside, 0.6);
		AdjustBrightness(layerrightlegside, 0.4);
		AdjustBrightness(layerheadside, 0.6);

		OverlayImage(SURFACE, leftarmside, cv::Point(0, 0));

		OverlayImage(SURFACE, headbottom, cv::Point(0, 0));
		OverlayImage(SURFACE, bodyside, cv::Point(0, 0));

		OverlayImage(SURFACE, layerleftarmside, cv::Point(0, 0));

		OverlayImage(SURFACE, bodyfront, cv::Point(0, 0));

		OverlayImage(SURFACE, HEAD_SHADOW, cv::Point(0, 0));

		OverlayImage(SURFACE, layerbodyfront, cv::Point(0, 0));
		OverlayImage(SURFACE, layerbodyside, cv::Point(0, 0));

		OverlayImage(SURFACE, headfront, cv::Point(0, 0));
		OverlayImage(SURFACE, headside, cv::Point(0, 0));

		OverlayImage(SURFACE, rightlegside, cv::Point(0, 0));
		OverlayImage(SURFACE, rightlegfront, cv::Point(0, 0));
		OverlayImage(SURFACE, rightarmside, cv::Point(0, 0));
		OverlayImage(SURFACE, rightarmfront, cv::Point(0, 0));
		OverlayImage(SURFACE, leftlegfront, cv::Point(0, 0));
		OverlayImage(SURFACE, leftarmfront, cv::Point(0, 0));

		OverlayImage(SURFACE, layerheadfront, cv::Point(0, 0));
		OverlayImage(SURFACE, layerheadside, cv::Point(0, 0));

		OverlayImage(SURFACE, LEG_SHADOW, cv::Point(0, 0));

		OverlayImage(SURFACE, layerrightlegside, cv::Point(0, 0));
		OverlayImage(SURFACE, layerrightarmfront, cv::Point(0, 0));
		OverlayImage(SURFACE, layerrightarmside, cv::Point(0, 0));
		OverlayImage(SURFACE, layerrightlegfront, cv::Point(0, 0));
		OverlayImage(SURFACE, layerleftlegfront, cv::Point(0, 0));
		OverlayImage(SURFACE, layerleftarmfront, cv::Point(0, 0));

		std::vector<cv::Mat> surface_channels;
		cv::split(SURFACE, surface_channels);

		cv::Mat OUT_LIGHTING;

		LIGHTING.copyTo(OUT_LIGHTING, surface_channels[3]);

		OverlayImage(SURFACE, OUT_LIGHTING, cv::Point(0, 0));

		return SURFACE;
	}
}

int main(int argc, char* argv[]) {

	auto print_usage = []() {
		std::cout << "Usage:" << std::endl;
		std::cout << "  SteveModMaker <minecraft_username> <slot_number> [arm_type]" << std::endl;
		std::cout << "  SteveModMaker --skin-file <skin_png_path> <slot_number> [arm_type]" << std::endl;
		std::cout << "  SteveModMaker -f <skin_png_path> <slot_number> [arm_type]" << std::endl;
		std::cout << "  slot_number: Costume slot (1-8)" << std::endl;
		std::cout << "  arm_type (optional): 'small' or 'big' to override auto-detection" << std::endl;
	};

	if (argc < 3) {
		print_usage();
		return -1;
	}

	try {

	bool use_skin_file = false;
	std::string skin_source;
	int slot_arg_index = 2;
	int arm_arg_index = 3;

	const std::string first_arg = argv[1];
	if (first_arg == "--skin-file" || first_arg == "-f") {
		if (argc < 4) {
			print_usage();
			return -1;
		}
		use_skin_file = true;
		skin_source = argv[2];
		slot_arg_index = 3;
		arm_arg_index = 4;
	}
	else {
		skin_source = argv[1];
	}

	// Parse slot number (1-8)
	int slot_input = std::stoi(argv[slot_arg_index]);
	if (slot_input < 1 || slot_input > 8) {
		std::cout << "Error: Slot number must be between 1 and 8" << std::endl;
		return -1;
	}
	
	uint8_t C0X_ = slot_input - 1; // Convert 1-8 to 0-7
	
	// Format as two-digit string (00-07)
	char buffer[3];
	snprintf(buffer, sizeof(buffer), "%02d", C0X_);
	std::string C0X(buffer);
	
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

	std::cout << "[SteveModMaker::Main] Determining player model..." << std::endl;
	bool model = DetectSlimModelFromSkin(skin);
	std::string arm_type = model ? "small" : "big";

	if (argc >= arm_arg_index + 1) {
		std::string forced_arm = argv[arm_arg_index];
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
	
	skin = ConvertToModernSkin(skin, model);

	std::cout << "[SteveModMaker::Main] Skin ready." << std::endl;

	cv::Mat base_render = CreateRender(skin, model);

	std::cout << "[SteveModMaker::Main] Created Render." << std::endl;

#ifdef _DEBUG
	imwrite("Final_Render.png", base_render);
#endif

	std::cout << "[SteveModMaker::Main] Creating output directories..." << std::endl;

	const std::filesystem::path target_slot = "./patch/fighter/pickel/model/body/c" + C0X;
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
		if (!WriteRequiredTemplate(arm_type, file_name, target_slot / file_name)) {
			std::cerr << "[SteveModMaker::Main] Error: Missing embedded fighter template file: " << file_name << std::endl;
			return -1;
		}
	}
	
	// Ensure UI directories exist for character select screen files
	std::filesystem::create_directories("./patch/ui/replace_patch/chara/chara_0");
	std::filesystem::create_directories("./patch/ui/replace_patch/chara/chara_1");
	std::filesystem::create_directories("./patch/ui/replace_patch/chara/chara_2");
	std::filesystem::create_directories("./patch/ui/replace_patch/chara/chara_3");
	std::filesystem::create_directories("./patch/ui/replace_patch/chara/chara_4");
	std::filesystem::create_directories("./patch/ui/replace_patch/chara/chara_5");
	std::filesystem::create_directories("./patch/ui/replace_patch/chara/chara_6");

	cv::Mat chara_0;
	cv::Mat chara_1;
	cv::Mat chara_4;

	cv::Mat chara_2 = LoadRequiredPng("Chara_Masks/chara_2_pickel_00.png");
	cv::Mat chara_4_mask = LoadRequiredPng("Chara_Masks/chara_4_mask.png");

	if (C0X_ % 2 == 0) { // Uses a steve slot.
		chara_0 = LoadRequiredPng("Chara_Masks/chara_0_pickel_00.png");
		chara_1 = LoadRequiredPng("Chara_Masks/chara_1_pickel_00.png");
		chara_4 = LoadRequiredPng("Chara_Masks/chara_4_pickel_00.png");
	}
	else {
		chara_0 = LoadRequiredPng("Chara_Masks/chara_0_pickel_01.png");
		chara_1 = LoadRequiredPng("Chara_Masks/chara_1_pickel_01.png");
		chara_4 = LoadRequiredPng("Chara_Masks/chara_4_pickel_01.png");
	}

	std::cout << "[SteveModMaker::Main] Creating chara_0 image..." << std::endl;
	{ // chara_0
		cv::Mat render_cpy(176, 336, CV_8UC4);
		cv::resize(base_render, render_cpy, cv::Size(176, 336), 0, 0, cv::INTER_LANCZOS4);
		OverlayImage(chara_0, render_cpy, cv::Point(-25, -2));

		BNTX bntx(chara_0, "chara_0_pickel_" + C0X);
		bntx.Write("./patch/ui/replace_patch/chara/chara_0/chara_0_pickel_" + C0X + ".bntx");
	}
	std::cout << "[SteveModMaker::Main] Creating chara_1 image..." << std::endl;
	{ // chara_1
		cv::Mat render_cpy(280, 540, CV_8UC4);
		cv::resize(base_render, render_cpy, cv::Size(280, 540), 0, 0, cv::INTER_LANCZOS4);
		OverlayImage(chara_1, render_cpy, cv::Point(93, 63));
		BNTX bntx(chara_1, "chara_1_pickel_" + C0X);
		bntx.Write("./patch/ui/replace_patch/chara/chara_1/chara_1_pickel_" + C0X + ".bntx");
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
		bntx.Write("./patch/ui/replace_patch/chara/chara_2/chara_2_pickel_" + C0X + ".bntx");
	}
	std::cout << "[SteveModMaker::Main] Creating chara_3 image..." << std::endl;
	{ // chara_3
		BNTX bntx(base_render, "chara_3_pickel_" + C0X);
		bntx.Write("./patch/ui/replace_patch/chara/chara_3/chara_3_pickel_" + C0X + ".bntx");
	}
	std::cout << "[SteveModMaker::Main] Creating chara_4 image..." << std::endl;
	{ // chara_4
		cv::Mat render_cpy(174, 333, CV_8UC4);
		cv::resize(base_render, render_cpy, cv::Size(174, 333), 0, 0, cv::INTER_LANCZOS4);
		OverlayImage(chara_4, render_cpy, cv::Point(-7, 8));
		Chara4Mask(chara_4, chara_4_mask);

		BNTX bntx(chara_4, "chara_4_pickel_" + C0X);
		bntx.Write("./patch/ui/replace_patch/chara/chara_4/chara_4_pickel_" + C0X + ".bntx");
	}
	// chara_5
	if (C0X_ == 0) {
		std::cout << "[SteveModMaker::Main] Creating chara_5 image..." << std::endl;
		cv::Mat chara_5 = LoadRequiredPng("Chara_Masks/chara_5_pickel_00.png");

		cv::Mat render_cpy(267, 527, CV_8UC4);
		cv::resize(base_render, render_cpy, cv::Size(267, 527), 0, 0, cv::INTER_LANCZOS4);
		OverlayImage(chara_5, render_cpy, cv::Point(4, -13));

		BNTX bntx(chara_5, "chara_5_pickel_00");
		bntx.Write("./patch/ui/replace_patch/chara/chara_5/chara_5_pickel_00.bntx");
	}
	else if (C0X_ == 1) {
		std::cout << "[SteveModMaker::Main] Creating chara_5 image..." << std::endl;
		cv::Mat chara_5 = LoadRequiredPng("Chara_Masks/chara_5_pickel_01.png");

		cv::Mat render_cpy(267, 527, CV_8UC4);
		cv::resize(base_render, render_cpy, cv::Size(267, 527), 0, 0, cv::INTER_LANCZOS4);
		OverlayImage(chara_5, render_cpy, cv::Point(-10, -14));

		BNTX bntx(chara_5, "chara_5_pickel_01");
		bntx.Write("./patch/ui/replace_patch/chara/chara_5/chara_5_pickel_01.bntx");
	}
	std::cout << "[SteveModMaker::Main] Creating chara_6 image..." << std::endl;
	{ // chara_6
		cv::Mat chara_6(256, 512, CV_8UC4); std::memset(chara_6.data, 0, 256 * 512 * 4);

		cv::Mat render_cpy(711, 1385, CV_8UC4);
		cv::resize(base_render, render_cpy, cv::Size(711, 1385), 0, 0, cv::INTER_LANCZOS4);
		OverlayImage(chara_6, render_cpy, cv::Point(-150, -110));

	 	BNTX bntx(chara_6, "chara_6_pickel_" + C0X);
		bntx.Write("./patch/ui/replace_patch/chara/chara_6/chara_6_pickel_" + C0X + ".bntx");
	}
	std::cout << "[SteveModMaker::Main] Writing fighter texture..." << std::endl;
	ColorCorrectSkin(skin);
	const std::filesystem::path output_nutexb = "./patch/fighter/pickel/model/body/c" + C0X + "/def_pickel_001_col.nutexb";
	const std::string base_template_key = std::string("Templates/") + (model ? "small_arms/" : "big_arms/") + "def_pickel_001_col.nutexb";
	const EmbeddedAssets::AssetView* base_asset = EmbeddedAssets::Find(base_template_key);

	NUTEXB nut;
	if (base_asset != nullptr && nut.Open(base_asset->data, base_asset->size) && nut.ReplaceTextureFromMat(skin)) {
		std::cout << "[SteveModMaker::Main] Using embedded def_pickel_001_col.nutexb template (" << arm_type << " arms)" << std::endl;
		nut.Save(output_nutexb, 0);
	}
	else {
		std::cout << "[SteveModMaker::Main] Base template not available, generating def_pickel_001_col.nutexb" << std::endl;
		NUTEXB generated("def_pickel_001_col", skin, NUTEXBFormat::R8G8B8A8_SRGB);
		generated.Save(output_nutexb, 0);
	}

	std::cout << "[SteveModMaker::Main] Done!" << std::endl;

	return 0;

	}
	catch (const std::exception& ex) {
		std::cerr << "[SteveModMaker::Main] Fatal error: " << ex.what() << std::endl;
		return -1;
	}
	
}
