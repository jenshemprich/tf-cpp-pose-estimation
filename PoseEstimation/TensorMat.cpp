#include "pch.h"

#include <opencv2/opencv.hpp>
#include <tensorflow/cc/framework/ops.h>

#include "GeometryOperators.h"
#include "TensorMat.h"

using namespace cv;
using namespace tensorflow;

tensorflow::ReAllocator::ReAllocator()
	: buffer(nullptr), size(0) {
}

tensorflow::ReAllocator::~ReAllocator() {
	if (buffer) {
		_aligned_free(buffer);
	}
}

string tensorflow::ReAllocator::Name()
{
	return string("TensorMat Re-Allocator");
}

void * tensorflow::ReAllocator::AllocateRaw(size_t alignment, size_t num_bytes) {
	if (buffer == nullptr) {
		buffer = _aligned_malloc(num_bytes, alignment);
		size = num_bytes;
	} else if (num_bytes > size) {
		buffer = _aligned_realloc(buffer, num_bytes, alignment);
		size = num_bytes;
	}
	return buffer;
}

void tensorflow::ReAllocator::DeallocateRaw(void * ptr) {
	// Freed on destruct
}


AffineTransform buffer2view_unit_interval(const Size& size, const Size& inset) {
	const Point2f limit = Point2f(inset + size + inset);
	return AffineTransform({
		Point2f(inset.width, inset.height) / limit,
		Point2f(inset.width + size.width, inset.height) / limit,
		Point2f(inset.width, inset.height + size.height) / limit }, {
		Point2f(0,0),
		Point2f(1.0, 0),
		Point2f(0, 1.0)
		});
}

TensorMat::TensorMat(const Size& size) : TensorMat(size, Size(0,0)) {
}

TensorMat::TensorMat(const Size& size, const Size& inset)
	: allocator()
	, tensorBuffer(&allocator, DT_FLOAT, TensorShape({ 1, inset.height + size.height + inset.height, inset.width + size.width + inset.width, 3 }))
	, tensor(tensorBuffer)
	, buffer(inset + size + inset, CV_32FC3, tensorBuffer.flat<float>().data())
	, size(size)
	, inset(inset)
	, view(buffer(Rect(inset, size)))
	, transform(buffer2view_unit_interval(size, inset)) {
		if (inset != Size(0, 0)) {
		buffer = 0;
	}
}

TensorMat::~TensorMat() {}

TensorMat& TensorMat::copyFrom(const Mat & mat) {
	if (size == mat.size()) {
		mat.convertTo(view, CV_32FC3);
	}
	else {
		Mat resized(size, mat.type());
		cv::resize(mat, resized, size);
		resized.convertTo(view, CV_32FC3);
	}

	return *this;
}

TensorMat& TensorMat::resize(const Size& size, const Size& inset) {
	tensorBuffer = tensorflow::Tensor(&allocator, DT_FLOAT, TensorShape({ 1, inset.height + size.height + inset.height, inset.width + size.width + inset.width, 3 }));
	buffer = Mat(inset + size + inset, CV_32FC3, tensorBuffer.flat<float>().data());
	view = buffer(Rect(inset, size));
	transform = AffineTransform(buffer2view_unit_interval(size, inset));
	this->size = size;
	this->inset = inset;

	return *this;
}
