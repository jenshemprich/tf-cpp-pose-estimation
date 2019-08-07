#pragma once

#include "opencv2/opencv.hpp"

#include "AffineTransform.h"
#include "CocoDataModel.h"

namespace coco {

	class OpenCvRenderer : public coco::Renderer {
	public:
		cv::Mat& image;
		const AffineTransform& input;
		const AffineTransform& view;

		OpenCvRenderer(cv::Mat& image, const AffineTransform& input, const AffineTransform& view);

		cv::Point2f transform(float x, float y);

		void joint(float x, float y, const int* rgb) override;

		void segment(float joint1_x, float joint1_y, float joint2_x, float joint2_y, const int* rgb) override;

	};

}
