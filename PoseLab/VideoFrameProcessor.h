#pragma once

#include <QVideoFrame>

class VideoFrameProcessor : public QObject {
	Q_OBJECT
public:
	VideoFrameProcessor(const std::function<void(QVideoFrame&)>&& function);

public slots:
	void process(QVideoFrame& frame) const;

private:
	const std::function<void(QVideoFrame&)> function;
};
