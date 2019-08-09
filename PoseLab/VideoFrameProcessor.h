#pragma once

#include <QVideoFrame>

class VideoFrameProcessor : public QObject {
	Q_OBJECT
public:
	VideoFrameProcessor(const Function&& function);

public slots:
	void process(QVideoFrame& frame) const;

private:
	const Function function;
};
