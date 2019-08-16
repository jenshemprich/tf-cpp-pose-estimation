#include "pch.h"

#include "MovieVideoFrameSource.h"

MovieVideoFrameSource::MovieVideoFrameSource(QThread& worker)
	: AbstractVideoFrameSource(worker), mediaPlayer(nullptr)
{
}

MovieVideoFrameSource::~MovieVideoFrameSource() {
	end();
	while (mediaPlayer != nullptr) {
		QCoreApplication::processEvents();
	}
}

void MovieVideoFrameSource::setPath(const QString& path) {
	this->path = path;
}

void MovieVideoFrameSource::startWork() {
	mediaPlayer = new QMediaPlayer();
	const QUrl url = QUrl::fromLocalFile(path);
	mediaPlayer->setMedia(url);
	mediaPlayer->setVideoOutput(surface);
	mediaPlayer->play();
}

void MovieVideoFrameSource::endWork() {
	if (mediaPlayer != nullptr) {
		mediaPlayer->stop();
		delete mediaPlayer;
		mediaPlayer = nullptr;
	}
}
