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

cv::Mat ConvertToModernSkin(cv::Mat& skin, bool model);

void ColorCorrectSkin(cv::Mat& skin);
