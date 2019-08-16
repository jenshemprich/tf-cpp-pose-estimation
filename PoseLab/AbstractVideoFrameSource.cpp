#include "pch.h"

#include <QAbstractVideoSurface>

#include "AbstractVideoFrameSource.h"

using namespace std;

AbstractVideoFrameSource::AbstractVideoFrameSource(QThread& worker)
	: QObject(nullptr), worker(worker), surface(nullptr)
{
	moveToThread(&worker);
}

AbstractVideoFrameSource::~AbstractVideoFrameSource() {
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
}
