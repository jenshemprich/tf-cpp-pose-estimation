#include "pch.h"

#include "tensorflow/cc/framework/ops.h"

#include "TensorMat.h"

using namespace cv;
using namespace tensorflow;

TensorMat::TensorMat(const cv::Size& size)
	: tensor(DT_FLOAT, TensorShape({ 1, size.height, size.width, 3 }))
	, buffer(size, CV_32FC3, tensor.flat<float>().data()) {
}

TensorMat::~TensorMat() {
}

tensorflow::Tensor& TensorMat::copyFrom(const cv::Mat & mat) {
	// TODO skip resize when size matches
	Mat resized(buffer.size(), mat.type());
	cv::resize(mat, resized, resized.size());
	resized.convertTo(buffer, CV_32FC3);
	return tensor;
}
