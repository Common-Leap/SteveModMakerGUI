#pragma once

#include <opencv2/opencv.hpp>

enum class PartSize : int {
	Head = 0,
	Size4x12 = 1,
	Size3x12 = 2,
	Body = 3,
};

enum class BodyType : int {
	STEVE = 4,
	ALEX = 3,
};

inline cv::Size CANVAS_SIZE(968, 1864);

inline cv::Point2f HEAD_SIZE[4] = {
	{0.0, 0.0},
	{800.0, 0.0},
	{800.0, 800.0},
	{0.0, 800.0}
};

inline cv::Point2f SIZE_4_12[4] = {
	{0.0, 0.0},
	{400.0, 0.0},
	{400.0, 1200.0},
	{0.0, 1200.0}
};

inline cv::Point2f SIZE_3_12[4] = {
	{0.0, 0.0},
	{300.0, 0.0},
	{300.0, 1200.0},
	{0.0, 1200.0}
};

inline cv::Point2f BODY_SIZE[4] = {
	{0.0, 0.0},
	{800.0, 0.0},
	{800.0, 1200.0},
	{0.0, 1200.0}
};
