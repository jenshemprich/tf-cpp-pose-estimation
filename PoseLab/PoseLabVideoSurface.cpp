#include "pch.h"

#include <QPixmap>

#include "PoseLabVideoSurface.h"

PoseLabVideoSurface::PoseLabVideoSurface(QObject *parent)
	: QAbstractVideoSurface(parent) {
}

PoseLabVideoSurface::~PoseLabVideoSurface() {
}

void PoseLabVideoSurface::setTarget(QLabel* target)
{
	myLabel = target;
}

QList<QVideoFrame::PixelFormat> PoseLabVideoSurface::supportedPixelFormats(QAbstractVideoBuffer::HandleType type) const {
	if (type == QAbstractVideoBuffer::NoHandle) {
		// TODO Camera provides RGBA but RGB is more than sufficient
		return QList<QVideoFrame::PixelFormat>() << QVideoFrame::Format_RGB32;
	}
	else {
		return QList<QVideoFrame::PixelFormat>();
	}
}

bool PoseLabVideoSurface::present(const QVideoFrame& frame) {
	// based on https://stackoverflow.com/questions/26229633/use-of-qabstractvideosurface
	// TODO Way too much cpu

	//if (notMyFormat(frame.pixelFormat())) {
	//	setError(IncorrectFormatError);
	//	return false;
	//}
	//else {
		QVideoFrame frametodraw(frame);

		if (!frametodraw.map(QAbstractVideoBuffer::ReadOnly))
		{
			setError(ResourceError);
			return false;
		}

		//this is a shallow operation. it just refer the frame buffer
		QImage image(
			frametodraw.bits(),
			frametodraw.width(),
			frametodraw.height(),
			frametodraw.bytesPerLine(),
			QImage::Format_ARGB32);

		myLabel->resize(image.size());

		//QPixmap::fromImage create a new buffer for the pixmap
		myLabel->setPixmap(QPixmap::fromImage(image));

		//we can release the data
		frametodraw.unmap();

		myLabel->update();

		return true;
//	}
}

