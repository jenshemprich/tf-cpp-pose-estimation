#pragma once

#include <QObject>
#include <QThread>

class QAbstractVideoSurface;
class QMediaPlayer;

class VideoFrameSource : public QObject {
	Q_OBJECT

public:
	VideoFrameSource(QObject *parent);
	~VideoFrameSource();

	void setPath(const QString& path);
	void setTarget(QAbstractVideoSurface* surface);

	void start();
	void end();

private:
signals:
	void start_();
	void end_();

private slots:
	void startWork();
	void endWork();

public:
	QThread worker;
private:
	QString path;
	QAbstractVideoSurface* surface;

	std::unique_ptr<QMediaPlayer> mediaPlayer;
};
