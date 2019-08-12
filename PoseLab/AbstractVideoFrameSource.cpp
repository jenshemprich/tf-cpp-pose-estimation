#include "pch.h"

#include <QAbstractVideoSurface>

#include "AbstractVideoFrameSource.h"

using namespace std;

AbstractVideoFrameSource::AbstractVideoFrameSource(QObject* parent)
	: QObject(parent), worker(this), surface(nullptr)
{
	worker.start();
	moveToThread(&worker);
}

AbstractVideoFrameSource::~AbstractVideoFrameSource() {
	worker.quit();
	worker.wait();
}

void AbstractVideoFrameSource::setTarget(QAbstractVideoSurface* surface) {
	this->surface = surface;
}

void AbstractVideoFrameSource::start() {
	connect(this, &AbstractVideoFrameSource::start_, this, &AbstractVideoFrameSource::startWork);
	emit start_();
	disconnect(this, &AbstractVideoFrameSource::start_, this, &AbstractVideoFrameSource::startWork);
	connect(this, &AbstractVideoFrameSource::end_, this, &AbstractVideoFrameSource::endWork);
}

void AbstractVideoFrameSource::end() {
	emit end_();
	disconnect(this, &AbstractVideoFrameSource::end_, this, &AbstractVideoFrameSource::endWork);
	// TODO Application won't exit after playing second movie
	// - same problem as in early dev when worker hadn't processed events until the media object was deleted
}
