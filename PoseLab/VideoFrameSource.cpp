#include "pch.h"

#include <QMediaPlayer>

#include "VideoFrameSource.h"

using namespace std;

VideoFrameSource::VideoFrameSource(QObject* parent)
	: QObject(parent), worker(this), surface(nullptr)
{
	worker.start();
	moveToThread(&worker);
	connect(this, &VideoFrameSource::start_, this, &VideoFrameSource::startWork);
	connect(this, &VideoFrameSource::end_, this, &VideoFrameSource::endWork);
}

VideoFrameSource::~VideoFrameSource() {
	end();

	while (mediaPlayer.get() != nullptr) {
		QCoreApplication::processEvents();
	}

	worker.quit();
	worker.wait();
}

void VideoFrameSource::setPath(const QString& path) {
	this->path = path;
}

void VideoFrameSource::setTarget(QAbstractVideoSurface* surface) {
	this->surface = surface;
}

void VideoFrameSource::start() {
	emit start_();
}

void VideoFrameSource::end() {
	emit end_();
}

void VideoFrameSource::startWork() {
	mediaPlayer = unique_ptr<QMediaPlayer>(new QMediaPlayer());
	const QUrl url = QUrl::fromLocalFile(path);
	mediaPlayer->setMedia(url);
	mediaPlayer->setVideoOutput(surface);
	mediaPlayer->play();
}

void VideoFrameSource::endWork() {
	if (mediaPlayer != nullptr) {
		mediaPlayer->stop();
		mediaPlayer = nullptr;
	}
}
