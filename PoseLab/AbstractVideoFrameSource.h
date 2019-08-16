#pragma once

#include <QObject>
#include <QThread>

class QAbstractVideoSurface;
class QMediaPlayer;

class AbstractVideoFrameSource : public QObject {
	Q_OBJECT

public:
	AbstractVideoFrameSource(QThread& worker);
	virtual ~AbstractVideoFrameSource();

	void setTarget(QAbstractVideoSurface* surface);

public slots:
	virtual void start();
	virtual void end();

protected:
signals:
	void start_();
	void end_();

protected slots:
	virtual void startWork()=0;
	virtual void endWork()=0;

protected:
	QAbstractVideoSurface* surface;
	QThread& worker;
private:
};
