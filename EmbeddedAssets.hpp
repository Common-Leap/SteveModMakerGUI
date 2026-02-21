#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

namespace EmbeddedAssets {

struct AssetView {
    const unsigned char* data;
    size_t size;
};

const AssetView* Find(const std::string& key);
cv::Mat LoadPng(const std::string& key);
bool WriteFile(const std::string& key, const std::filesystem::path& output_path);

}  // namespace EmbeddedAssets
