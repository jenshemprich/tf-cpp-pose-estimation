#pragma once

#include <opencv2/opencv.hpp>

#include <QThread>
#include <QTimer>

#include "AbstractVideoFrameSource.h"

class OpenCVFFMpegSource : public AbstractVideoFrameSource {
	Q_OBJECT

public:
	OpenCVFFMpegSource(QThread& worker);
	virtual ~OpenCVFFMpegSource();

	void setPath(const QString& path);

	virtual void startWork() override;
	virtual void endWork() override;

private slots:
	void presentFrame();

protected:
	QString path;

	QTimer* timer;
	cv::VideoCapture* videoCapture;
	cv::Mat frame;

	void present(cv::Mat& frame);
};
