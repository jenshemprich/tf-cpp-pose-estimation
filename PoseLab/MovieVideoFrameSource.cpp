#include "pch.h"

#include "MovieVideoFrameSource.h"

using namespace std;

MovieVideoFrameSource::MovieVideoFrameSource(QObject* parent)
	: AbstractVideoFrameSource(parent)
{
}

MovieVideoFrameSource::~MovieVideoFrameSource() {
	end();

	while (mediaPlayer.get() != nullptr) {
		QCoreApplication::processEvents();
	}
}

void MovieVideoFrameSource::setPath(const QString& path) {
	this->path = path;
}

void MovieVideoFrameSource::startWork() {
	mediaPlayer = unique_ptr<QMediaPlayer>(new QMediaPlayer());
	const QUrl url = QUrl::fromLocalFile(path);
	mediaPlayer->setMedia(url);
	mediaPlayer->setVideoOutput(surface);
	mediaPlayer->play();
}

void MovieVideoFrameSource::endWork() {
	if (mediaPlayer != nullptr) {
		mediaPlayer->stop();
		mediaPlayer = nullptr;
	}
}
