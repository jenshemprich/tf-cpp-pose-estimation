#pragma once

#include "AffineTransform.h"

class TensorMat {
public:
	TensorMat(const cv::Size& size);
	TensorMat(const cv::Size& size, const cv::Size& inset);
	virtual ~TensorMat();

	TensorMat& copyFrom(const cv::Mat& mat);
	tensorflow::Tensor tensor;
	const cv::Size size;
	const cv::Size inset;
	const AffineTransform transform;
private:
	cv::Mat buffer;
	cv::Mat view;
};
