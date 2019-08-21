#include "pch.h"

#include <opencv2/opencv.hpp>

#include "OpenGlVideoSurface.h"

using namespace cv;

OpenGlVideoSurface::OpenGlVideoSurface(OpenGlVideoView* display)
	: view(display)
{}

QList<QVideoFrame::PixelFormat> OpenGlVideoSurface::supportedPixelFormats(QAbstractVideoBuffer::HandleType type) const {
	if (type == QAbstractVideoBuffer::NoHandle) {
		// TODO QCamera provides RGB32 only but RGB24 would be more than sufficient
		return QList<QVideoFrame::PixelFormat>() << QVideoFrame::Format_RGB32 << QVideoFrame::Format_BGR24;
	}
	else {
		return QList<QVideoFrame::PixelFormat>();
	}
}

bool OpenGlVideoSurface::present(const QVideoFrame& frame) {
	QVideoFrame localFrame = frame;
	if (localFrame.map(QAbstractVideoBuffer::ReadOnly)) {
		if (format.scanLineDirection() == QVideoSurfaceFormat::Direction::BottomToTop) {
			// TODO forward video surface format and vflip in video processor and OpenGL view
			Mat frameRef(localFrame.height(), localFrame.width(), CV_8UC4, localFrame.bits(), localFrame.bytesPerLine());
			Mat tmp;
			flip(frameRef, tmp, 0);
			tmp.copyTo(frameRef);
		}

		emit frameArrived(localFrame);
		emit aboutToPresent(localFrame);
		localFrame.unmap();
		return true;
	}
	else {
		return false;
	}
}

bool OpenGlVideoSurface::start(const QVideoSurfaceFormat& surfaceFormat) {
	format = surfaceFormat;
	emit surfaceFormatChanged(surfaceFormat);
	return true;
}

void OpenGlVideoSurface::stop() {
}

