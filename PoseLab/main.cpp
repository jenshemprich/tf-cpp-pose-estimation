#include "pch.h"

#include <QtWidgets/QApplication>
#include <QCamera>
#include <QVideoSurfaceFormat>
#include <QCameraInfo>

#include "PoseLab.h"

using namespace std;

int main(int argc, char *argv[]) {
	QApplication a(argc, argv);
	PoseLab poseLab;
	poseLab.show();

	unique_ptr<QCamera> camera;
	QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
	foreach(const QCameraInfo & cameraInfo, cameras) {
		//const QString description = cameraInfo.description();
		//if (description.contains("Front")) {
		if (cameraInfo.position() == QCamera::Position::FrontFace) {
				camera = make_unique<QCamera>(new QCamera(cameraInfo));
		}
	}

	// TODO Cameras don't seem to have a position on Surface 4 
	// TODO Camera is alwys rear, no matter what (description says front)
	if (camera == nullptr) {
		camera = make_unique<QCamera>(new QCamera(cameras[0]));
	}

	// auto camera = make_unique<QCamera>(new QCamera(QCamera::FrontFace));
	// auto camera = make_unique<QCamera>(new QCamera);
	camera->setViewfinder(poseLab.videoSurface);
	camera->start();

	return a.exec();
}
