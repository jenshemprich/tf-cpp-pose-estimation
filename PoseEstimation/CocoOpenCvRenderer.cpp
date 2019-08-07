#include "pch.h"

#include "CocoOpenCvRenderer.h"

namespace coco {
	
	using namespace cv;

	OpenCvRenderer::OpenCvRenderer(cv::Mat& image, const AffineTransform& input, const AffineTransform& view)
		: image(image), input(input), view(view) {}

	Point2f OpenCvRenderer::transform(float x, float y) {
		// TODO concat transforms on caller side to reduce method parameters 
		const Point2f normalized = input({ x, y });
		return view(normalized);
	}

	void OpenCvRenderer::joint(float x, float y, const int* rgb) {
		const Scalar_<int> color(rgb[0], rgb[1], rgb[2]);
		circle(image, transform(x, y), 3, color, 3, 8, 0);
	}

	 void OpenCvRenderer::segment(float joint1_x, float joint1_y, float joint2_x, float joint2_y, const int* rgb) {
		const Scalar_<int> color(rgb[0], rgb[1], rgb[2]);
		line(image, transform(joint1_x, joint1_y), transform(joint2_x, joint2_y), color, 3);
	}

}