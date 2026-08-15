#pragma once

#include <iostream>
#include <opencv2/opencv.hpp>

void CurlDownloadToOSTREAM(const std::string& url, std::ostream& os);

cv::Mat CurlDownloadToMat(const std::string& url);

std::string CurlDownloadToString(const std::string& url);

// Returns true if the skin uses the Alex playermodel, and false if the skin uses the Steve playermodel.
bool GetModel(const std::string& username);

// Detects Alex (slim) vs Steve (classic) directly from skin texture layout.
bool DetectSlimModelFromSkin(const cv::Mat& skin);

cv::Mat DownloadSkin(const std::string& username);

cv::Mat LoadSkinFromFile(const std::string& file_path);

// Loads the player's current Minecraft cape texture from Mojang's profile.
cv::Mat DownloadCape(const std::string& username);

// Loads a canonical Minecraft cape texture from a local PNG file.
cv::Mat LoadCapeFromFile(const std::string& file_path);

// Loads one of the official cape atlases embedded in the application.
cv::Mat LoadOfficialCape(const std::string& cape_id);

cv::Mat ConvertToModernSkin(cv::Mat& skin, bool model);

// Applies the established Smash model-texture treatment while preserving alpha.
void ColorCorrectModelTexture(cv::Mat& texture);
