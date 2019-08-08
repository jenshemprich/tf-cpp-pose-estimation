#include "pch.h"

#include "OpenGlVideoSurface.h"

OpenGlVideoSurface::OpenGlVideoSurface(OpenGlVideoView* display)
	: view(display)
{}

QList<QVideoFrame::PixelFormat> OpenGlVideoSurface::supportedPixelFormats(QAbstractVideoBuffer::HandleType type) const {
	if (type == QAbstractVideoBuffer::NoHandle) {
		// TODO QCamera provides RGB32 only but RGB24 would be more than sufficient
		return QList<QVideoFrame::PixelFormat>() << QVideoFrame::Format_RGB32;
	}
	else {
		return QList<QVideoFrame::PixelFormat>();
	}
}

bool OpenGlVideoSurface::present(const QVideoFrame& frame) {
	QVideoFrame localFrame = frame;
	if (localFrame.map(QAbstractVideoBuffer::ReadOnly)) {
		emit frameArrived(localFrame);
		emit aboutToPresent(localFrame);
		localFrame.unmap();
		return true;
	}
	else {
		return false;
	}
}

