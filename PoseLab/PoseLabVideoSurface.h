#pragma once

#include <QAbstractVideoSurface>
#include <QLabel>

class PoseLabVideoSurface : public QAbstractVideoSurface
{
	Q_OBJECT

public:
	PoseLabVideoSurface(QObject *parent);
	~PoseLabVideoSurface();

	void setTarget(QLabel* target);

	// Inherited via QAbstractVideoSurface
	virtual QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType type = QAbstractVideoBuffer::NoHandle) const override;
	virtual bool present(const QVideoFrame& frame) override;
private:
	QLabel* myLabel;
};
