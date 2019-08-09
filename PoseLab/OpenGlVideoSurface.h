#pragma once

#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>

class OpenGlVideoView;

class OpenGlVideoSurface : public QAbstractVideoSurface {
	Q_OBJECT

public:
	OpenGlVideoSurface(OpenGlVideoView* display);

	virtual QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType type = QAbstractVideoBuffer::NoHandle) const override;
	virtual bool present(const QVideoFrame& frame) override;
	virtual bool start(const QVideoSurfaceFormat& surfaceFormat) override;
	virtual void stop() override;

signals:
	void frameArrived(QVideoFrame& frame);
	void aboutToPresent(QVideoFrame& frame);
private:
	OpenGlVideoView* view;
	QVideoSurfaceFormat format;
};
