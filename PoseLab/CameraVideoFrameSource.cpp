#include "pch.h"

#include "CameraVideoFrameSource.h"

using namespace std;

CameraVideoFrameSource::CameraVideoFrameSource(const QCameraInfo& cameraInfo, QObject* parent)
	: AbstractVideoFrameSource(parent), cameraInfo(cameraInfo)
{
}

CameraVideoFrameSource::~CameraVideoFrameSource() {
	end();

	while (camera.get() != nullptr) {
		QCoreApplication::processEvents();
	}
}

void CameraVideoFrameSource::startWork() {
	camera = unique_ptr<QCamera>(new QCamera());
	QCameraViewfinderSettings settings(camera->viewfinderSettings());
	settings.setResolution(640, 480);
	camera->setViewfinderSettings(settings);
	camera->setViewfinder(surface);
	camera->start();
}

void CameraVideoFrameSource::endWork() {
	if (camera != nullptr) {
		camera->stop();
		camera = nullptr;
	}
}
