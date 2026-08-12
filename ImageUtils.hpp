#pragma once

#include <opencv2/opencv.hpp>
#include "Constants.hpp"

struct RGBA {
	unsigned char R;
	unsigned char G;
	unsigned char B;
	unsigned char A;
};

void AlphaBlendColors(RGBA& BottomPixel, const RGBA& TopPixel);

cv::Mat RenderPerspectiveTransformation(float topleftx, float toplefty, float toprightx, float toprighty, float bottomrightx, float bottomrighty, float bottomleftx, float bottomlefty, PartSize bodypart, cv::Mat& SOURCE_IMAGE);

cv::Mat CropAndScale(cv::Mat& skin, cv::Rect rect);

void OverlayImage(cv::Mat& background, const cv::Mat& foreground, cv::Point2i location);

void Chara4Mask(cv::Mat& surface, cv::Mat& mask);
