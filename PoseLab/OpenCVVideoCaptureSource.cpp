#include "pch.h"

#include <QSize>
#include <QVideoFrame>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>

#include "OpenCVVideoBuffer.h"

#include "OpenCVVideoCaptureSource.h"

using namespace cv;
using namespace std;

OpenCVVideoCaptureSource::OpenCVVideoCaptureSource(const QString& deviceName, QThread& worker)
	: AbstractVideoFrameSource(worker), timer(nullptr), deviceName(deviceName), videoCapture(nullptr)
{}

OpenCVVideoCaptureSource::~OpenCVVideoCaptureSource() {
	end();
	while (videoCapture != nullptr) {
		QCoreApplication::processEvents();
	}
}

void OpenCVVideoCaptureSource::startWork() { // Ignored
	videoCapture = new VideoCapture;
	int device = deviceName.toInt();
	if (videoCapture->open(device, CAP_DSHOW)) {
		*videoCapture >> frame;
		QVideoSurfaceFormat surfaceFormat(QSize(frame.cols, frame.rows), QVideoFrame::PixelFormat::Format_BGR24, QAbstractVideoBuffer::NoHandle);
		surface->start(surfaceFormat);
		present(frame);
		timer = new QTimer(this);
		timer->moveToThread(QThread::currentThread());
		connect(timer, &QTimer::timeout, this, &OpenCVVideoCaptureSource::presentFrame);
		timer->start();
	} else {
		assert(false);
	}
}

void OpenCVVideoCaptureSource::presentFrame() {
	assert(videoCapture);
	if (videoCapture) {
		*videoCapture >> frame;
		present(frame);
		timer->start();
	}
}

void OpenCVVideoCaptureSource::present(cv::Mat& frame) {
	OpenCVVideoBuffer* buffer = new OpenCVVideoBuffer(frame);
	QVideoFrame videoFrame(buffer, QSize(frame.cols, frame.rows), QVideoFrame::PixelFormat::Format_BGR24);
	surface->present(videoFrame);
}

void OpenCVVideoCaptureSource::endWork() { // Ignored
	if (timer) {
		timer->stop();
		disconnect(timer, &QTimer::timeout, this, &OpenCVVideoCaptureSource::presentFrame);
		delete timer;
		timer = nullptr;
		surface->stop();
	}

	if (videoCapture) {
		videoCapture->release();
		delete videoCapture;
		videoCapture = nullptr;
	}
}
