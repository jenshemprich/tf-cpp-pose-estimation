#pragma once

#include <QObject>
#include <QThread>

class QAbstractVideoSurface;
class QMediaPlayer;

class AbstractVideoFrameSource : public QObject {
	Q_OBJECT

public:
	AbstractVideoFrameSource(QObject *parent);
	~AbstractVideoFrameSource();

	void setTarget(QAbstractVideoSurface* surface);

	void start();
	void end();

private:
signals:
	void start_();
	void end_();

private slots:
	virtual void startWork()=0;
	virtual void endWork()=0;

protected:
	QAbstractVideoSurface* surface;
private:
	QThread worker;
};
