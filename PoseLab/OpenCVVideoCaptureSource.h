#pragma once

#include <opencv2/opencv.hpp>

#include <QThread>
#include <QTimer>

#include "AbstractVideoFrameSource.h"

class OpenCVVideoCaptureSource : public AbstractVideoFrameSource {
	Q_OBJECT

public:
	OpenCVVideoCaptureSource(const QString& deviceName, QThread& worker);
	virtual ~OpenCVVideoCaptureSource();

	virtual void startWork() override;
	virtual void endWork() override;

private slots:
	void presentFrame();

private:
	const QString deviceName;
	
	QTimer* timer;
	cv::VideoCapture* videoCapture;
	cv::Mat frame;

	void present(cv::Mat& frame);
};
