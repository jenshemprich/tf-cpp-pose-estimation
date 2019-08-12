#pragma once

#include <QMediaPlayer>

#include "AbstractVideoFrameSource.h"

class MovieVideoFrameSource : public AbstractVideoFrameSource
{
	Q_OBJECT

public:
	MovieVideoFrameSource(QObject* parent);
	virtual ~MovieVideoFrameSource();

	void setPath(const QString& path);

	virtual void startWork() override;
	virtual void endWork() override;

private:
	QString path;
	std::unique_ptr<QMediaPlayer> mediaPlayer;
};
