#pragma once

#include <QMediaPlayer>

#include "AbstractVideoFrameSource.h"

class MovieVideoFrameSource : public AbstractVideoFrameSource {
public:
	MovieVideoFrameSource(QThread& worker);
	virtual ~MovieVideoFrameSource();

	void setPath(const QString& path);

	virtual void startWork() override;
	virtual void endWork() override;

private:
	QString path;
	QMediaPlayer* mediaPlayer;
};
