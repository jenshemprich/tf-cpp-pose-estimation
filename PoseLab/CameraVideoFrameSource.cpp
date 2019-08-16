#include "pch.h"

#include "CameraVideoFrameSource.h"

CameraVideoFrameSource::CameraVideoFrameSource(const QString& deviceName, QThread& worker)
	: AbstractVideoFrameSource(worker), deviceName(deviceName), camera(nullptr)
{
}

CameraVideoFrameSource::~CameraVideoFrameSource() {
	end();
	while (camera != nullptr) {
		QCoreApplication::processEvents();
	}
}

void CameraVideoFrameSource::startWork() {
	camera = new QCamera(deviceName.toLocal8Bit());
	QCameraViewfinderSettings settings(camera->viewfinderSettings());
	settings.setResolution(640, 480);
	camera->setViewfinderSettings(settings);
	camera->setViewfinder(surface);
	camera->start();
}

void CameraVideoFrameSource::endWork() {
	if (camera != nullptr) {
		camera->stop();
		delete camera;
		camera = nullptr;
	}
}
