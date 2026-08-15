#include <vector>
#include <iostream>
#include <filesystem>
#include <curl/curl.h>
#include <rapidjson/document.h>

#include "Base64.hpp"
#include "CapeCatalog.hpp"
#include "CapeLayout.hpp"
#include "EmbeddedAssets.hpp"
#include "ImageUtils.hpp"
#include "MinecraftSkinUtil.hpp"

namespace {

constexpr long kCurlConnectTimeoutSec = 15L;
constexpr long kCurlTotalTimeoutSec = 60L;
constexpr const char* kCurlUserAgent = "SteveModMaker/1.0";

bool TryConvertToBGRA(const cv::Mat& input, cv::Mat& output) {
	if (input.empty()) {
		return false;
	}
	if (input.depth() != CV_8U) {
		return false;
	}

	switch (input.channels()) {
	case 4:
		output = input.clone();
		return true;
	case 3:
		cv::cvtColor(input, output, cv::COLOR_BGR2BGRA);
		return true;
	case 1:
		cv::cvtColor(input, output, cv::COLOR_GRAY2BGRA);
		return true;
	default:
		return false;
	}
}

cv::Mat NormalizeSkinForTool(const cv::Mat& raw_skin, const std::string& source_label) {
	cv::Mat skin_bgra;
	if (!TryConvertToBGRA(raw_skin, skin_bgra)) {
		std::cerr << "Error: Unsupported skin image format from " << source_label << std::endl;
		return cv::Mat();
	}

	if (skin_bgra.cols == 64 && (skin_bgra.rows == 32 || skin_bgra.rows == 64)) {
		return skin_bgra;
	}

	// Support HD skins by reducing to the canonical Minecraft skin dimensions.
	if (skin_bgra.cols == skin_bgra.rows && (skin_bgra.cols % 64 == 0)) {
		cv::Mat resized;
		cv::resize(skin_bgra, resized, cv::Size(64, 64), 0, 0, cv::INTER_NEAREST);
		return resized;
	}
	if (skin_bgra.cols == skin_bgra.rows * 2 && (skin_bgra.cols % 64 == 0)) {
		cv::Mat resized;
		cv::resize(skin_bgra, resized, cv::Size(64, 32), 0, 0, cv::INTER_NEAREST);
		return resized;
	}

	std::cerr << "Error: Unsupported skin dimensions from " << source_label
		<< ": " << skin_bgra.cols << "x" << skin_bgra.rows
		<< ". Expected 64x64, 64x32, or HD multiples." << std::endl;
	return cv::Mat();
}

cv::Mat NormalizeCapeForTool(const cv::Mat& raw_cape, const std::string& source_label) {
	cv::Mat cape_bgra;
	if (!TryConvertToBGRA(raw_cape, cape_bgra)) {
		std::cerr << "Error: Unsupported cape image format from " << source_label << std::endl;
		return cv::Mat();
	}

	// Minecraft cape files use a 64x32 atlas. The decorated outside is the
	// 10x16-pixel face at atlas coordinates x=12..21, y=1..16. The adjacent
	// x=1..10 face is the inside of the worn cape.
	if (cape_bgra.cols == CapeLayout::kAtlasWidth && cape_bgra.rows == CapeLayout::kAtlasHeight) {
		return cape_bgra;
	}

	// Preserve pixel-art edges when accepting an integer-scaled cape atlas.
	if (cape_bgra.cols == cape_bgra.rows * 2 && cape_bgra.cols % CapeLayout::kAtlasWidth == 0) {
		cv::Mat resized;
		cv::resize(
			cape_bgra,
			resized,
			cv::Size(CapeLayout::kAtlasWidth, CapeLayout::kAtlasHeight),
			0,
			0,
			cv::INTER_NEAREST
		);
		return resized;
	}

	std::cerr << "Error: Unsupported cape dimensions from " << source_label
		<< ": " << cape_bgra.cols << "x" << cape_bgra.rows
		<< ". Expected Minecraft's 64x32 cape atlas or an integer-scaled equivalent." << std::endl;
	return cv::Mat();
}

bool ConfigureCurlCommonOptions(CURL* handle, const std::string& url) {
	if (handle == nullptr) {
		return false;
	}

	curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, kCurlConnectTimeoutSec);
	curl_easy_setopt(handle, CURLOPT_TIMEOUT, kCurlTotalTimeoutSec);
	curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(handle, CURLOPT_USERAGENT, kCurlUserAgent);
	return true;
}

bool CurlRequestSucceeded(CURL* handle, CURLcode result, std::string& error_out) {
	if (result != CURLE_OK) {
		error_out = curl_easy_strerror(result);
		return false;
	}

	long response_code = 0;
	if (curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &response_code) != CURLE_OK) {
		error_out = "failed to read HTTP response code";
		return false;
	}
	if (response_code < 200 || response_code >= 300) {
		error_out = "HTTP " + std::to_string(response_code);
		return false;
	}
	return true;
}

} // namespace

static size_t CurlWriteCallbackVectorBuffer(void* contents, size_t _size, size_t nmemb, void* userp) {
	size_t size = _size * nmemb;

	std::vector<uint8_t>& vec = *(std::vector<uint8_t>*)userp;

	size_t old_size = vec.size();

	vec.resize(vec.size() + size);

	memcpy(&vec[old_size], contents, size);

	return size;
}

static size_t CurlWriteCallbackString(void* contents, size_t size, size_t nmemb, void* userp)
{
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

static size_t CurlWriteCallbackOSTREAM(void* buf, size_t size, size_t nmemb, void* userp)
{
	std::ostream& os = *static_cast<std::ostream*>(userp);
	std::streamsize len = size * nmemb;
	if (os.write(static_cast<char*>(buf), len))
		return len;

	return 0;
}

void CurlDownloadToOSTREAM(const std::string& url, std::ostream& os)
{
	CURL* easyhandle = curl_easy_init();
	if (!ConfigureCurlCommonOptions(easyhandle, url)) {
		std::cerr << "Error: Failed to initialize CURL handle for " << url << std::endl;
		return;
	}

	curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, &CurlWriteCallbackOSTREAM);
	curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, &os);
	std::string error;
	const CURLcode result = curl_easy_perform(easyhandle);
	if (!CurlRequestSucceeded(easyhandle, result, error)) {
		std::cerr << "Error: Download failed for " << url << ": " << error << std::endl;
	}

	curl_easy_cleanup(easyhandle);
}

cv::Mat CurlDownloadToMat(const std::string& url)
{
	CURL* easyhandle = curl_easy_init();
	if (!ConfigureCurlCommonOptions(easyhandle, url)) {
		std::cerr << "Error: Failed to initialize CURL handle for " << url << std::endl;
		return cv::Mat();
	}

	std::vector<unsigned char> vec;
	curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, &CurlWriteCallbackVectorBuffer);
	curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, &vec);
	std::string error;
	const CURLcode result = curl_easy_perform(easyhandle);
	if (!CurlRequestSucceeded(easyhandle, result, error)) {
		std::cerr << "Error: Download failed for " << url << ": " << error << std::endl;
		curl_easy_cleanup(easyhandle);
		return cv::Mat();
	}

	curl_easy_cleanup(easyhandle);
	if (vec.empty()) {
		std::cerr << "Error: Empty response body for " << url << std::endl;
		return cv::Mat();
	}
	return cv::imdecode(vec, cv::IMREAD_UNCHANGED);
}

std::string CurlDownloadToString(const std::string& url) {

	std::string ret;

	CURL* easyhandle = curl_easy_init();
	if (!ConfigureCurlCommonOptions(easyhandle, url)) {
		std::cerr << "Error: Failed to initialize CURL handle for " << url << std::endl;
		return {};
	}

	curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, CurlWriteCallbackString);

	curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, &ret);
	std::string error;
	const CURLcode result = curl_easy_perform(easyhandle);
	if (!CurlRequestSucceeded(easyhandle, result, error)) {
		std::cerr << "Error: Download failed for " << url << ": " << error << std::endl;
		curl_easy_cleanup(easyhandle);
		return {};
	}

	curl_easy_cleanup(easyhandle);

	return ret;
}

bool LoadTextureProfileDocument(const std::string& username, rapidjson::Document& texture_document) {
	std::string res = CurlDownloadToString("https://api.mojang.com/users/profiles/minecraft/" + username);

	rapidjson::Document profile_document;
	profile_document.Parse(res.c_str());
	if (profile_document.HasParseError()) {
		std::cerr << "Error: Failed to parse player profile response: " << res << std::endl;
		return false;
	}
	if (!profile_document.IsObject() || !profile_document.HasMember("id") || !profile_document["id"].IsString()) {
		std::cerr << "Error: Invalid player profile response for " << username << std::endl;
		return false;
	}

	const std::string uuid = profile_document["id"].GetString();
	res = CurlDownloadToString("https://sessionserver.mojang.com/session/minecraft/profile/" + uuid);
	profile_document.Parse(res.c_str());
	if (profile_document.HasParseError()) {
		std::cerr << "Error: Failed to parse session response" << std::endl;
		return false;
	}
	if (!profile_document.IsObject() || !profile_document.HasMember("properties") ||
		!profile_document["properties"].IsArray() || profile_document["properties"].Size() == 0) {
		std::cerr << "Error: Invalid session response or no properties found" << std::endl;
		return false;
	}
	if (!profile_document["properties"][0].IsObject() || !profile_document["properties"][0].HasMember("value") ||
		!profile_document["properties"][0]["value"].IsString()) {
		std::cerr << "Error: Invalid property data" << std::endl;
		return false;
	}

	const std::string decoded = base64_decode(profile_document["properties"][0]["value"].GetString());
	texture_document.Parse(decoded.c_str());
	if (texture_document.HasParseError()) {
		std::cerr << "Error: Failed to parse decoded texture data" << std::endl;
		return false;
	}
	if (!texture_document.IsObject() || !texture_document.HasMember("textures") || !texture_document["textures"].IsObject()) {
		std::cerr << "Error: No textures found in profile" << std::endl;
		return false;
	}

	return true;
}

// Returns true if the skin uses the Alex playermodel, and false if the skin uses the Steve playermodel.
bool GetModel(const std::string& username) {
	rapidjson::Document document;
	if (!LoadTextureProfileDocument(username, document)) {
		return false;
	}

	if (document["textures"].HasMember("SKIN") && document["textures"]["SKIN"].IsObject() &&
		document["textures"]["SKIN"].HasMember("metadata") && document["textures"]["SKIN"]["metadata"].IsObject() &&
		document["textures"]["SKIN"]["metadata"].HasMember("model") && document["textures"]["SKIN"]["metadata"]["model"].IsString())
		return (std::string)document["textures"]["SKIN"]["metadata"]["model"].GetString() == "slim";
	else
		return false;
}

bool DetectSlimModelFromSkin(const cv::Mat& skin) {
	if (skin.empty() || skin.cols < 64 || skin.rows < 32) {
		return false;
	}

	auto alpha_at = [&](int x, int y) -> uint8_t {
		if (x < 0 || y < 0 || x >= skin.cols || y >= skin.rows) {
			return 255;
		}
		if (skin.channels() < 4) {
			return 255;
		}
		return skin.ptr<uint8_t>(y)[x * skin.channels() + 3];
	};

	// Common slim markers in modern skin layout.
	const bool marker_right_arm = (alpha_at(54, 20) == 0);
	const bool marker_left_arm = (alpha_at(46, 52) == 0);

	return marker_right_arm || marker_left_arm;
}

cv::Mat DownloadSkin(const std::string& username) {
	rapidjson::Document document;
	if (!LoadTextureProfileDocument(username, document)) {
		return cv::Mat();
	}

	if (!document["textures"].HasMember("SKIN") || !document["textures"]["SKIN"].IsObject() ||
		!document["textures"]["SKIN"].HasMember("url") || !document["textures"]["SKIN"]["url"].IsString()) {
		std::cerr << "Error: No skin texture URL found" << std::endl;
		return cv::Mat();
	}

	const std::string URL = document["textures"]["SKIN"]["url"].GetString();

	cv::Mat result = NormalizeSkinForTool(CurlDownloadToMat(URL), "downloaded skin");
	if (result.empty()) {
		std::cerr << "Error: Failed to download skin texture" << std::endl;
	}
	return result;
}

cv::Mat LoadSkinFromFile(const std::string& file_path) {
	const std::filesystem::path path(file_path);
	if (!std::filesystem::exists(path)) {
		std::cerr << "Error: Skin file does not exist: " << file_path << std::endl;
		return cv::Mat();
	}

	cv::Mat raw = cv::imread(path.string(), cv::IMREAD_UNCHANGED);
	if (raw.empty()) {
		std::cerr << "Error: Failed to read skin file: " << file_path << std::endl;
		return cv::Mat();
	}

	return NormalizeSkinForTool(raw, "file '" + file_path + "'");
}

cv::Mat DownloadCape(const std::string& username) {
	rapidjson::Document document;
	if (!LoadTextureProfileDocument(username, document)) {
		return cv::Mat();
	}

	if (!document["textures"].HasMember("CAPE") || !document["textures"]["CAPE"].IsObject() ||
		!document["textures"]["CAPE"].HasMember("url") || !document["textures"]["CAPE"]["url"].IsString()) {
		std::cerr << "Error: No Minecraft cape is associated with " << username << std::endl;
		return cv::Mat();
	}

	const std::string URL = document["textures"]["CAPE"]["url"].GetString();
	cv::Mat result = NormalizeCapeForTool(CurlDownloadToMat(URL), "downloaded Minecraft cape");
	if (result.empty()) {
		std::cerr << "Error: Failed to download cape texture" << std::endl;
	}
	return result;
}

cv::Mat LoadCapeFromFile(const std::string& file_path) {
	const std::filesystem::path path(file_path);
	if (!std::filesystem::exists(path)) {
		std::cerr << "Error: Cape file does not exist: " << file_path << std::endl;
		return cv::Mat();
	}

	cv::Mat raw = cv::imread(path.string(), cv::IMREAD_UNCHANGED);
	if (raw.empty()) {
		std::cerr << "Error: Failed to read cape file: " << file_path << std::endl;
		return cv::Mat();
	}

	return NormalizeCapeForTool(raw, "file '" + file_path + "'");
}

cv::Mat LoadOfficialCape(const std::string& cape_id) {
	const CapeCatalogEntry* entry = FindOfficialCape(cape_id);
	if (entry == nullptr) {
		std::cerr << "Error: Unknown official cape: " << cape_id << std::endl;
		return cv::Mat();
	}

	const cv::Mat raw = EmbeddedAssets::LoadPng(OfficialCapeResourceKey(*entry));
	if (raw.empty()) {
		std::cerr << "Error: Missing embedded cape texture: " << entry->name << std::endl;
		return cv::Mat();
	}
	return NormalizeCapeForTool(raw, "embedded official cape '" + std::string(entry->name) + "'");
}

cv::Mat ConvertToModernSkin(cv::Mat& skin, bool model) {
	if (skin.rows == skin.cols)
		return skin;

	cv::Mat ret(64, 64, CV_8UC4);
	skin.copyTo(ret(cv::Rect(0, 0, 64, 32)));

	if (model) { // Alex
		cv::flip(skin(cv::Rect(40, 20, 4, 12)), ret(cv::Rect(39, 52, 4, 12)), 1); // ArmSide1
		cv::flip(skin(cv::Rect(44, 20, 3, 12)), ret(cv::Rect(36, 52, 3, 12)), 1); // ArmSide2
		cv::flip(skin(cv::Rect(47, 20, 4, 12)), ret(cv::Rect(32, 52, 4, 12)), 1); // ArmSide3
		cv::flip(skin(cv::Rect(44, 20, 3, 12)), ret(cv::Rect(43, 52, 3, 12)), 1); // ArmSide4
		cv::flip(skin(cv::Rect(44, 16, 3, 4)), ret(cv::Rect(36, 48, 3, 4)), 1); // ArmBTM
		cv::flip(skin(cv::Rect(47, 16, 3, 4)), ret(cv::Rect(39, 48, 3, 4)), 1); // ArmTOP

	}
	else {
		cv::flip(skin(cv::Rect(40, 20, 4, 12)), ret(cv::Rect(40, 52, 4, 12)), 1); // ArmSide1
		cv::flip(skin(cv::Rect(44, 20, 4, 12)), ret(cv::Rect(36, 52, 4, 12)), 1); // ArmSide2
		cv::flip(skin(cv::Rect(48, 20, 4, 12)), ret(cv::Rect(32, 52, 4, 12)), 1); // ArmSide3
		cv::flip(skin(cv::Rect(52, 20, 4, 12)), ret(cv::Rect(44, 52, 4, 12)), 1); // ArmSide4
		cv::flip(skin(cv::Rect(44, 16, 4, 4)), ret(cv::Rect(36, 48, 4, 4)), 1); // ArmBTM
		cv::flip(skin(cv::Rect(48, 16, 4, 4)), ret(cv::Rect(40, 48, 4, 4)), 1); // ArmTOP
	}

	cv::flip(skin(cv::Rect(0, 20, 4, 12)), ret(cv::Rect(24, 52, 4, 12)), 1); // LegSide1
	cv::flip(skin(cv::Rect(4, 20, 4, 12)), ret(cv::Rect(20, 52, 4, 12)), 1); // LegSide2
	cv::flip(skin(cv::Rect(8, 20, 4, 12)), ret(cv::Rect(16, 52, 4, 12)), 1); // LegSide3
	cv::flip(skin(cv::Rect(12, 20, 4, 12)), ret(cv::Rect(28, 52, 4, 12)), 1); // LegSide4
	cv::flip(skin(cv::Rect(4, 16, 4, 4)), ret(cv::Rect(20, 48, 4, 4)), 1); // LegBTM
	cv::flip(skin(cv::Rect(8, 16, 4, 4)), ret(cv::Rect(24, 48, 4, 4)), 1); // LegTOP

	return ret;
}

void ColorCorrectModelTexture(cv::Mat& texture) {
	if (texture.empty() || texture.type() != CV_8UC4) {
		return;
	}

	// Values traditionally used by Minecraft-to-Smash skin converters: gamma
	// 1.2 and a 75% highlight ceiling reduce Ultimate's bloom and glare.
	constexpr double gamma_power = 1.0 / 1.2;
	constexpr double highlight_ceiling = 191.0;

	for (int y = 0; y < texture.rows; ++y) {
		uint8_t* row = texture.ptr<uint8_t>(y);
		for (int x = 0; x < texture.cols; ++x) {
			uint8_t* channels = row + x * 4;

			// OpenCV uses BGRA for CV_8UC4. Apply correction to B,G,R only and preserve alpha.
			for (int i = 0; i < 3; i++) {
				double corrected = std::pow((double)channels[i] / 255.0, gamma_power) * highlight_ceiling;
				channels[i] = static_cast<uint8_t>(corrected);
			}
		}
	}
}
