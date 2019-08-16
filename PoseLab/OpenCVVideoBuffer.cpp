#include "pch.h"

#include "OpenCVVideoBuffer.h"

using namespace cv;

OpenCVVideoBuffer::OpenCVVideoBuffer(cv::Mat& mat)
	: QAbstractVideoBuffer(QAbstractVideoBuffer::NoHandle), mat(mat), mapped(0)
{
}

OpenCVVideoBuffer::~OpenCVVideoBuffer() {
	assert(!mapped);
}

uchar* OpenCVVideoBuffer::map(QAbstractVideoBuffer::MapMode mode, int* numBytes, int* bytesPerLine) {
	this->mode = mode;
	mapped++;
	*numBytes = mat.total() * mat.elemSize();
	*bytesPerLine = mat.cols * mat.elemSize();
	return static_cast<uchar*>(mat.data);
}

void OpenCVVideoBuffer::unmap() {
	mapped--;
	mode = mapped ? mode : QAbstractVideoBuffer::MapMode::NotMapped;
}

QAbstractVideoBuffer::MapMode OpenCVVideoBuffer::mapMode() const {
	return mode;
}
