#include "pch.h"

#include <QVideoFrame>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>

#include "OpenCVVideoBuffer.h"

#include "OpenCVFFMpegSource.h"

// TODO Duplicated code -> opencv unifies camera and video access in VideoCapture -> refactor to base class and provide control object to generate overlays

using namespace cv;
using namespace std;

OpenCVFFMpegSource::OpenCVFFMpegSource(QThread& worker)
	: AbstractVideoFrameSource(worker), timer(nullptr), videoCapture(nullptr)
{}

OpenCVFFMpegSource::~OpenCVFFMpegSource() {
	end();
	while (videoCapture != nullptr) {
		QCoreApplication::processEvents();
	}
}

void OpenCVFFMpegSource::setPath(const QString& path) {
	this->path = path;
}

void OpenCVFFMpegSource::startWork() {
	videoCapture = new VideoCapture;
	if (videoCapture->open(path.toStdString())) {
		*videoCapture >> frame;
		QVideoSurfaceFormat surfaceFormat(QSize(frame.cols, frame.rows), QVideoFrame::PixelFormat::Format_BGR24, QAbstractVideoBuffer::NoHandle);
		surface->start(surfaceFormat);
		present(frame);
		timer = new QTimer(this);
		timer->moveToThread(QThread::currentThread());
		connect(timer, &QTimer::timeout, this, &OpenCVFFMpegSource::presentFrame);
		timer->start();
	} else {
		assert(false);
	}
}

void OpenCVFFMpegSource::presentFrame() {
	assert(videoCapture);
	if (videoCapture) {
		*videoCapture >> frame;
		present(frame);
	}
}

void OpenCVFFMpegSource::present(cv::Mat& frame) {
	OpenCVVideoBuffer* buffer = new OpenCVVideoBuffer(frame);
	QVideoFrame videoFrame(buffer, QSize(frame.cols, frame.rows), QVideoFrame::PixelFormat::Format_BGR24);
	surface->present(videoFrame);
}

void OpenCVFFMpegSource::endWork() {
	if (timer) {
		timer->stop();
		disconnect(timer, &QTimer::timeout, this, &OpenCVFFMpegSource::presentFrame);
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
