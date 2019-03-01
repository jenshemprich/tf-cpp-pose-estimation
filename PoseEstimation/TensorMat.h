#pragma once

#include <opencv2/opencv.hpp>
#include <tensorflow/cc/framework/ops.h>

class TensorMat {
public:
	TensorMat(const cv::Size& size);
	TensorMat(const cv::Size& size, const cv::Size& inset);
	virtual ~TensorMat();

	TensorMat& copyFrom(const cv::Mat& mat);
	tensorflow::Tensor tensor;
	const cv::Size size;
	const cv::Size inset;
	const cv::Mat transform;
private:
	cv::Mat buffer;
	cv::Mat view;
};
