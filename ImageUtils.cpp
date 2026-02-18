#include "ImageUtils.hpp"

// https://stackoverflow.com/questions/45751605/pixels-overlay-with-transparency
void AlphaBlendColors(RGBA& BottomPixel, const RGBA& TopPixel) {
	// Calculate new Alpha:
	float normA1 = 0.003921568627451f * (TopPixel.A);
	float normA2 = 0.003921568627451f * (BottomPixel.A);

	unsigned char newAlpha = (unsigned char)((normA1 + normA2 * (1.0f - normA1)) * 255.0f);

	if (newAlpha == 0) {
		BottomPixel.R = 0;
		BottomPixel.G = 0;
		BottomPixel.B = 0;
		BottomPixel.A = 0;
	}

	// Going By Straight Alpha formula
	float dstCoef = normA2 * (1.0f - normA1);
	float multiplier = 255.0f / float(newAlpha);

	BottomPixel.R = (unsigned char)((TopPixel.R * normA1 + BottomPixel.R * dstCoef) * multiplier);
	BottomPixel.G = (unsigned char)((TopPixel.G * normA1 + BottomPixel.G * dstCoef) * multiplier);
	BottomPixel.B = (unsigned char)((TopPixel.B * normA1 + BottomPixel.B * dstCoef) * multiplier);

	BottomPixel.A = newAlpha;
}

cv::Mat RenderPerspectiveTransformation(float topleftx, float toplefty, float toprightx, float toprighty, float bottomrightx, float bottomrighty, float bottomleftx, float bottomlefty, PartSize bodypart, cv::Mat& SOURCE_IMAGE) {
	cv::Point2f objectivePoints[4]{};
	cv::Point2f imagePoints[4] = {
		{topleftx , toplefty},
		{toprightx, toprighty},
		{bottomrightx, bottomrighty},
		{bottomleftx, bottomlefty}
	};
	cv::Mat transform{};
	switch (bodypart) {
	case PartSize::Head:
		transform = getPerspectiveTransform(imagePoints, HEAD_SIZE);
		break;
	case PartSize::Size4x12:
		transform = getPerspectiveTransform(imagePoints, SIZE_4_12);
		break;
	case PartSize::Size3x12:
		transform = getPerspectiveTransform(imagePoints, SIZE_3_12);
		break;
	case PartSize::Body:
		transform = getPerspectiveTransform(imagePoints, BODY_SIZE);
		break;
	}

	cv::warpPerspective(SOURCE_IMAGE, SOURCE_IMAGE, transform, CANVAS_SIZE, cv::INTER_CUBIC | cv::WARP_INVERSE_MAP);

	return SOURCE_IMAGE;
}

cv::Mat CropAndScale(cv::Mat& skin, cv::Rect rect) {
	cv::Mat cropped = skin(rect);
	cv::Mat scaled(cropped.cols * 100, cropped.rows * 100, CV_8UC4);
	cv::resize(cropped, scaled, cv::Size(cropped.cols * 100, cropped.rows * 100), 0, 0, cv::INTER_NEAREST);
	return scaled;
}

void OverlayImage(cv::Mat& background, const cv::Mat& foreground, cv::Point2i location) {

	assert(background.channels() == 4);
	assert(foreground.channels() == 4);

	// start at the row indicated by location, or at row 0 if location.y is negative.
	for (int y = std::max(location.y, 0); y < background.rows; ++y) {
		int fY = y - location.y; // because of the translation

		// we are done of we have processed all rows of the foreground image.
		if (fY >= foreground.rows)
			break;

		// start at the column indicated by location, or at column 0 if location.x is negative.
		for (int x = std::max(location.x, 0); x < background.cols; ++x) {
			int fX = x - location.x; // because of the translation.

			// we are done with this row if the column is outside of the foreground image.
			if (fX >= foreground.cols)
				break;

			RGBA& foregroundPx = *(RGBA*)&foreground.data[fY * foreground.step + fX * 4];
			RGBA& backgroundPx = *(RGBA*)&background.data[y * background.step + x * 4];

			AlphaBlendColors(backgroundPx, foregroundPx);
		}
	}
}

void AdjustBrightness(cv::Mat& surface, double factor)
{
	for (int y = 0; y < surface.rows; ++y) {
		for (int x = 0; x < surface.cols; ++x) {
			surface.data[y * surface.step + 4 * x] = surface.data[y * surface.step + 4 * x] * factor;
			surface.data[y * surface.step + 4 * x + 1] = surface.data[y * surface.step + 4 * x + 1] * factor;
			surface.data[y * surface.step + 4 * x + 2] = surface.data[y * surface.step + 4 * x + 2] * factor;
			// surface.data[y * surface.step + 4 * x + 3] = surface.data[y * surface.step + 4 * x + 3] * factor;
		}
	}
}

void Chara4Mask(cv::Mat& surface, cv::Mat& mask)
{
	for (int y = 0; y < surface.rows; ++y) {
		for (int x = 0; x < surface.cols; ++x) {
			RGBA& colorPixel = *(RGBA*)&surface.data[y * surface.step + x * 4];
			RGBA& maskPixel = *(RGBA*)&mask.data[y * mask.step + x * 4];
			colorPixel.A = colorPixel.A * (maskPixel.R / 255.0f);
		}
	}
}
