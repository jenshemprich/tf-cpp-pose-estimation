#include "pch.h"

#include <QAbstractVideoSurface>

#include "AbstractVideoFrameSource.h"

using namespace std;

AbstractVideoFrameSource::AbstractVideoFrameSource(QThread& worker)
	: QObject(nullptr), worker(worker), surface(nullptr)
{
	moveToThread(&worker);
	connect(this, &AbstractVideoFrameSource::start_, this, &AbstractVideoFrameSource::startWork);
	connect(this, &AbstractVideoFrameSource::end_, this, &AbstractVideoFrameSource::endWork);
}

AbstractVideoFrameSource::~AbstractVideoFrameSource() {
	disconnect(this, &AbstractVideoFrameSource::start_, this, &AbstractVideoFrameSource::startWork);
	disconnect(this, &AbstractVideoFrameSource::end_, this, &AbstractVideoFrameSource::endWork);
}

void AbstractVideoFrameSource::setTarget(QAbstractVideoSurface* surface) {
	this->surface = surface;
}

void AbstractVideoFrameSource::start() {
	emit start_();
}

void AbstractVideoFrameSource::end() {
	emit end_();
	// TODO Application won't exit after playing second movie
	// - same problem as in early dev when worker hadn't processed events until the media object was deleted
}
