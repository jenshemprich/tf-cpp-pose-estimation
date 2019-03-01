#include "pch.h"

#include "tensorflow/cc/framework/ops.h"

#include "TensorMat.h"

using namespace cv;
using namespace tensorflow;

Point2f operator/(const Point2f& a, const Point2f& b) {
	return Point2f(a.x / b.x, a.y / b.y);
}

Mat buffer2view_unit_interval(const Size& size, const Size& inset) {
	const Point2f limit = Point2f(inset + size + inset);
	const Point2f buffer[] = {
		Point2f(inset.width, inset.height) / limit,
		Point2f(inset.width + size.width, inset.height) / limit,
		Point2f(inset.width, inset.height + size.height) / limit };

	const Point2f view[] = {
		Point2f(0,0),
		Point2f(1.0, 0),
		Point2f(0, 1.0)
	};

	return getAffineTransform(buffer, view);
}

TensorMat::TensorMat(const Size& size)
	: tensor(DT_FLOAT, TensorShape({ 1, size.height, size.width, 3 }))
	, buffer(size, CV_32FC3, tensor.flat<float>().data())
	, view(buffer(Rect(inset, size)))
	, size(size)
	, inset()
	, transform(buffer2view_unit_interval(size, inset))
	{}

TensorMat::TensorMat(const Size& size, const Size& inset)
	: tensor(DT_FLOAT, TensorShape({ 1, inset.height + size.height + inset.height, inset.width + size.width + inset.width, 3 }))
	, buffer(inset + size + inset, CV_32FC3, tensor.flat<float>().data())
	, size(size)
	, inset(inset)
	, view(buffer(Rect(inset, size)))
	, transform(buffer2view_unit_interval(size, inset)) {
		buffer = 0;
}

TensorMat::~TensorMat() {}

TensorMat& TensorMat::copyFrom(const Mat & mat) {
	// TODO skip resize when size matches
	Mat resized(size, mat.type());
	resize(mat, resized, size);
	resized.convertTo(view, CV_32FC3);
	return *this;
}
