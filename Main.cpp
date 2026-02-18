#include <iostream>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "Nutexb.hpp"
#include "BNTX.hpp"
#include "Constants.hpp"
#include "ImageUtils.hpp"
#include "MinecraftSkinUtil.hpp"

cv::Mat CreateRender(cv::Mat& skin, bool model) {

	if (model)
	{
		std::string res_path(RESOURCE_PATH);
		cv::Mat HEAD_SHADOW = cv::imread(res_path + "/HEAD_SHADOW.png", cv::IMREAD_UNCHANGED);
		cv::Mat LEG_SHADOW = cv::imread(res_path + "/LEG_SHADOW.png", cv::IMREAD_UNCHANGED);
		cv::Mat LIGHTING = cv::imread(res_path + "/LIGHTING_EXP.png", cv::IMREAD_UNCHANGED);

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
		std::string res_path(RESOURCE_PATH);
		cv::Mat HEAD_SHADOW = cv::imread(res_path + "/HEAD_SHADOW.png", cv::IMREAD_UNCHANGED);
		cv::Mat LEG_SHADOW = cv::imread(res_path + "/LEG_SHADOW.png", cv::IMREAD_UNCHANGED);
		cv::Mat LIGHTING = cv::imread(res_path + "/LIGHTING_EXP.png", cv::IMREAD_UNCHANGED);

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

	if (argc < 3) {
		std::cout << "Usage: SteveModMaker <minecraft_username> <slot_number> [arm_type]" << std::endl;
		std::cout << "  minecraft_username: Your Minecraft Java Edition username" << std::endl;
		std::cout << "  slot_number: Costume slot (1-8)" << std::endl;
		std::cout << "  arm_type: 'big' for big arms (default) or 'small' for small arms" << std::endl;
		return -1;
	}

	// Parse slot number (1-8)
	int slot_input = std::stoi(argv[2]);
	if (slot_input < 1 || slot_input > 8) {
		std::cout << "Error: Slot number must be between 1 and 8" << std::endl;
		return -1;
	}
	
	uint8_t C0X_ = slot_input - 1; // Convert 1-8 to 0-7
	
	// Format as two-digit string (00-07)
	char buffer[3];
	snprintf(buffer, sizeof(buffer), "%02d", C0X_);
	std::string C0X(buffer);
	
	// Parse arm type (big or small, default: big)
	std::string arm_type = "big";
	if (argc >= 4) {
		arm_type = argv[3];
		if (arm_type != "big" && arm_type != "small") {
			std::cout << "Error: arm_type must be 'big' or 'small'" << std::endl;
			return -1;
		}
	}
	std::cout << "[SteveModMaker::Main] Using " << arm_type << " arms template" << std::endl;

	std::cout << "[SteveModMaker::Main] Downloading skin for " << argv[1] << "..." << std::endl;
	cv::Mat skin = DownloadSkin(argv[1]);
	if (skin.empty()) {
		std::cerr << "[SteveModMaker::Main] Failed to download skin" << std::endl;
		return -1;
	}

	std::cout << "[SteveModMaker::Main] Determining player model..." << std::endl;
	bool model = GetModel(argv[1]);
	
	skin = ConvertToModernSkin(skin, model);

	std::cout << "[SteveModMaker::Main] Downloaded Skin." << std::endl;

	cv::Mat base_render = CreateRender(skin, model);

	std::cout << "[SteveModMaker::Main] Created Render." << std::endl;

#ifdef _DEBUG
	imwrite("Final_Render.png", base_render);
#endif

	std::cout << "[SteveModMaker::Main] Creating output directories..." << std::endl;

	// Copy fighter template files from arm type templates
	std::string template_dir = "/home/leap/Workshop/fighter/pickel/model/body/c[00-07] (" + arm_type + " arms)";
	std::string target_slot = "./patch/fighter/pickel/model/body/c" + C0X;
	
	if (std::filesystem::exists(template_dir)) {
		std::cout << "[SteveModMaker::Main] Copying fighter template files..." << std::endl;
		std::filesystem::create_directories(target_slot);
		
		// Copy all template files except the texture
		for (const auto& entry : std::filesystem::directory_iterator(template_dir)) {
			std::string filename = entry.path().filename().string();
			if (filename != "def_pickel_001_col.nutexb") {
				std::filesystem::copy_file(entry.path(), target_slot + "/" + filename, std::filesystem::copy_options::overwrite_existing);
			}
		}
	} else {
		std::filesystem::create_directories(target_slot);
	}
	
	// Ensure UI directories exist for character select screen files
	std::filesystem::create_directories("./patch/ui/replace_patch/chara/chara_0");
	std::filesystem::create_directories("./patch/ui/replace_patch/chara/chara_1");
	std::filesystem::create_directories("./patch/ui/replace_patch/chara/chara_2");
	std::filesystem::create_directories("./patch/ui/replace_patch/chara/chara_3");
	std::filesystem::create_directories("./patch/ui/replace_patch/chara/chara_4");
	std::filesystem::create_directories("./patch/ui/replace_patch/chara/chara_6");

	cv::Mat chara_0;
	cv::Mat chara_1;
	cv::Mat chara_4;

	std::string masks_path(CHARA_MASKS_PATH);
	cv::Mat chara_2 = cv::imread(masks_path + "/chara_2_pickel_00.png", cv::IMREAD_UNCHANGED);
	cv::Mat chara_4_mask = cv::imread(masks_path + "/chara_4_mask.png", cv::IMREAD_UNCHANGED);

	if (C0X_ % 2 == 0) { // Uses a steve slot.
		chara_0 = cv::imread(masks_path + "/chara_0_pickel_00.png", cv::IMREAD_UNCHANGED);
		chara_1 = cv::imread(masks_path + "/chara_1_pickel_00.png", cv::IMREAD_UNCHANGED);
		chara_4 = cv::imread(masks_path + "/chara_4_pickel_00.png", cv::IMREAD_UNCHANGED);
	}
	else {
		chara_0 = cv::imread(masks_path + "/chara_0_pickel_01.png", cv::IMREAD_UNCHANGED);
		chara_1 = cv::imread(masks_path + "/chara_1_pickel_01.png", cv::IMREAD_UNCHANGED);
		chara_4 = cv::imread(masks_path + "/chara_4_pickel_01.png", cv::IMREAD_UNCHANGED);
	}

	std::cout << "[SteveModMaker::Main] Creating chara_0 image..." << std::endl;
	{ // chara_0
		cv::Mat render_cpy(176, 336, CV_8UC4);
		cv::resize(base_render, render_cpy, cv::Size(176, 336), 0, 0, cv::INTER_AREA);
		OverlayImage(chara_0, render_cpy, cv::Point(-25, -2));

		BNTX bntx(chara_0, "chara_0_pickel_" + C0X);
		bntx.Write("./patch/ui/replace_patch/chara/chara_0/chara_0_pickel_" + C0X + ".bntx");
	}
	std::cout << "[SteveModMaker::Main] Creating chara_1 image..." << std::endl;
	{ // chara_1
		cv::Mat render_cpy(280, 540, CV_8UC4);
		cv::resize(base_render, render_cpy, cv::Size(280, 540), 0, 0, cv::INTER_AREA);
		OverlayImage(chara_1, render_cpy, cv::Point(93, 63));
		BNTX bntx(chara_1, "chara_1_pickel_" + C0X);
		bntx.Write("./patch/ui/replace_patch/chara/chara_1/chara_1_pickel_" + C0X + ".bntx");
	}
	std::cout << "[SteveModMaker::Main] Creating chara_2 image..." << std::endl;
	{ // chara_2
		cv::Mat head = skin(cv::Rect(8, 8, 8, 8));
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
		cv::resize(base_render, render_cpy, cv::Size(174, 333), 0, 0, cv::INTER_AREA);
		OverlayImage(chara_4, render_cpy, cv::Point(-7, 8));
		Chara4Mask(chara_4, chara_4_mask);

		BNTX bntx(chara_4, "chara_4_pickel_" + C0X);
		bntx.Write("./patch/ui/replace_patch/chara/chara_4/chara_4_pickel_" + C0X + ".bntx");
	}
	// chara_5
	if (C0X_ == 0) {
		std::cout << "[SteveModMaker::Main] Creating chara_5 image..." << std::endl;
		cv::Mat chara_5 = cv::imread(masks_path + "/chara_5_pickel_00.png", cv::IMREAD_UNCHANGED);

		cv::Mat render_cpy(267, 527, CV_8UC4);
		cv::resize(base_render, render_cpy, cv::Size(267, 527), 0, 0, cv::INTER_AREA);
		OverlayImage(chara_5, render_cpy, cv::Point(4, -13));

		BNTX bntx(chara_5, "chara_5_pickel_00");
		bntx.Write("./patch/ui/replace_patch/chara/chara_5/chara_5_pickel_00.bntx");
	}
	else if (C0X_ == 1) {
		std::cout << "[SteveModMaker::Main] Creating chara_5 image..." << std::endl;
		cv::Mat chara_5 = cv::imread(masks_path + "/chara_5_pickel_01.png", cv::IMREAD_UNCHANGED);

		cv::Mat render_cpy(267, 527, CV_8UC4);
		cv::resize(base_render, render_cpy, cv::Size(267, 527), 0, 0, cv::INTER_AREA);
		OverlayImage(chara_5, render_cpy, cv::Point(-10, -14));

		BNTX bntx(chara_5, "chara_5_pickel_01");
		bntx.Write("./patch/ui/replace_patch/chara/chara_5/chara_5_pickel_01.bntx");
	}
	std::cout << "[SteveModMaker::Main] Creating chara_6 image..." << std::endl;
	{ // chara_6
		cv::Mat chara_6(256, 512, CV_8UC4); std::memset(chara_6.data, 0, 256 * 512 * 4);

		cv::Mat render_cpy(711, 1385, CV_8UC4);
		cv::resize(base_render, render_cpy, cv::Size(711, 1385), 0, 0, cv::INTER_AREA);
		OverlayImage(chara_6, render_cpy, cv::Point(-150, -110));

	 	BNTX bntx(chara_6, "chara_6_pickel_" + C0X);
		bntx.Write("./patch/ui/replace_patch/chara/chara_6/chara_6_pickel_" + C0X + ".bntx");
	}
	std::cout << "[SteveModMaker::Main] Writing fighter texture..." << std::endl;
	ColorCorrectSkin(skin);
	NUTEXB nut("def_pickel_001_col", skin, NUTEXBFormat::R8G8B8A8_SRGB);
	nut.Save("./patch/fighter/pickel/model/body/c" + C0X + "/def_pickel_001_col.nutexb", 0x1CB0); // Pads NUTEXB to 23728 bytes

	std::cout << "[SteveModMaker::Main] Done!" << std::endl;

	return 0;
	
}