#pragma once

#include <opencv2/opencv.hpp>
#include <tensorflow/cc/framework/ops.h>

class TensorMat {
public:
	TensorMat(const cv::Size& size);
	virtual ~TensorMat();

	tensorflow::Tensor& copyFrom(const cv::Mat& mat);
	tensorflow::Tensor tensor;
private:
	cv::Mat buffer;
};
