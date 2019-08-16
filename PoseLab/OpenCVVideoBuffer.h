#pragma once

#include <opencv2/opencv.hpp>

#include <QAbstractVideoBuffer>

class OpenCVVideoBuffer : public QAbstractVideoBuffer {
public:
	OpenCVVideoBuffer(cv::Mat& mat);
	virtual ~OpenCVVideoBuffer();

	virtual uchar* map(QAbstractVideoBuffer::MapMode mode, int* numBytes, int* bytesPerLine) override;
	virtual void unmap() override;
	virtual QAbstractVideoBuffer::MapMode mapMode() const override;
private:
	cv::Mat& mat;
	int mapped;
	QAbstractVideoBuffer::MapMode mode;
};

