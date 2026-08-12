#include "ImageUtils.hpp"

#include <algorithm>

#if CV_VERSION_MAJOR >= 5
// OpenCV 5 moved getPerspectiveTransform out of imgproc into the geometry module.
#include <opencv2/geometry/2d.hpp>
#endif

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
		return;
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
	constexpr float kSupersample = 2.0f;
	cv::Point2f imagePoints[4] = {
		{topleftx * kSupersample, toplefty * kSupersample},
		{toprightx * kSupersample, toprighty * kSupersample},
		{bottomrightx * kSupersample, bottomrighty * kSupersample},
		{bottomleftx * kSupersample, bottomlefty * kSupersample}
	};
	cv::Mat transform{};
	switch (bodypart) {
	case PartSize::Head:
		transform = cv::getPerspectiveTransform(imagePoints, HEAD_SIZE);
		break;
	case PartSize::Size4x12:
		transform = cv::getPerspectiveTransform(imagePoints, SIZE_4_12);
		break;
	case PartSize::Size3x12:
		transform = cv::getPerspectiveTransform(imagePoints, SIZE_3_12);
		break;
	case PartSize::Body:
		transform = cv::getPerspectiveTransform(imagePoints, BODY_SIZE);
		break;
	}

	// Warp premultiplied color so the transparent border contributes no black
	// fringe to the antialiased silhouette. Convert back to straight alpha for
	// the existing compositor after interpolation.
	const float inv255 = 1.0f / 255.0f;
	cv::Mat premultiplied(SOURCE_IMAGE.size(), CV_32FC4);
	for (int y = 0; y < SOURCE_IMAGE.rows; ++y) {
		const cv::Vec4b* source_row = SOURCE_IMAGE.ptr<cv::Vec4b>(y);
		cv::Vec4f* premultiplied_row = premultiplied.ptr<cv::Vec4f>(y);
		for (int x = 0; x < SOURCE_IMAGE.cols; ++x) {
			const cv::Vec4b& source_pixel = source_row[x];
			const float alpha = source_pixel[3] * inv255;
			premultiplied_row[x] = cv::Vec4f(
				source_pixel[0] * inv255 * alpha,
				source_pixel[1] * inv255 * alpha,
				source_pixel[2] * inv255 * alpha,
				alpha
			);
		}
	}

	cv::Mat warped;
	cv::warpPerspective(
		premultiplied,
		warped,
		transform,
		cv::Size(
			static_cast<int>(CANVAS_SIZE.width * kSupersample),
			static_cast<int>(CANVAS_SIZE.height * kSupersample)
		),
		cv::INTER_LINEAR | cv::WARP_INVERSE_MAP,
		cv::BORDER_CONSTANT,
		cv::Scalar(0, 0, 0, 0)
	);

	cv::Mat downsampled;
	cv::resize(warped, downsampled, CANVAS_SIZE, 0, 0, cv::INTER_AREA);

	SOURCE_IMAGE.create(CANVAS_SIZE, CV_8UC4);
	for (int y = 0; y < SOURCE_IMAGE.rows; ++y) {
		const cv::Vec4f* warped_row = downsampled.ptr<cv::Vec4f>(y);
		cv::Vec4b* output_row = SOURCE_IMAGE.ptr<cv::Vec4b>(y);
		for (int x = 0; x < SOURCE_IMAGE.cols; ++x) {
			const cv::Vec4f& warped_pixel = warped_row[x];
			const float alpha = std::clamp(warped_pixel[3], 0.0f, 1.0f);
			if (alpha <= 1.0e-6f) {
				output_row[x] = cv::Vec4b(0, 0, 0, 0);
				continue;
			}
			output_row[x] = cv::Vec4b(
				cv::saturate_cast<uint8_t>(warped_pixel[0] / alpha * 255.0f),
				cv::saturate_cast<uint8_t>(warped_pixel[1] / alpha * 255.0f),
				cv::saturate_cast<uint8_t>(warped_pixel[2] / alpha * 255.0f),
				cv::saturate_cast<uint8_t>(alpha * 255.0f)
			);
		}
	}

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
