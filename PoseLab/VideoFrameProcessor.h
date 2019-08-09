#pragma once

#include <QVideoFrame>

class VideoFrameProcessor : public QObject {
	Q_OBJECT
public:
	typedef std::function<void(QVideoFrame&)> Function;

	VideoFrameProcessor(const Function&& function);

public slots:
	void process(QVideoFrame& frame) const;

private:
	const Function function;
};
