#include "pch.h"

#include <QtWidgets/QApplication>
#include <QCamera>
#include <QVideoSurfaceFormat>
#include <QCameraInfo>
#include <QVideoWidget>

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

	// How do they perform?
	// OpenCV eats up 50-60% percent cpu and no gpu when streaming HD from the back camera 
	camera->setCaptureMode(QCamera::CaptureVideo);

	// 42% cpu
	// auto camera = make_unique<QCamera>(new QCamera(QCamera::FrontFace));
	// auto camera = make_unique<QCamera>(new QCamera);
	//camera->setViewfinder(poseLab.videoSurface);
	//camera->start();

	// Using QVideoView reduces cpu down from 42% to 33%, but still too much
	// Also how to limit frame rate
	// camera->setViewfinder(poseLab.videoWidget.get());
	// camera->start();

	// Using OpenGL with Vertex and frsgmant shader redM$ Camera uces
	// - cpu to 28%
	// + gpu to 43% which is quite a lot
	camera->setViewfinder(poseLab.openGLvideoView->surface);
	camera->start();
	const QSize resolution = camera.get()->viewfinderSettings().resolution();
	poseLab.openGLvideoView->setFixedSize(resolution);

	// M$ Camera app has 3% cpu + 8% GPU wich is what we want too
	// M$ camera app uses GPU video module which is a little different from 3D

	// Okay, better than OpenCV but with high GPU load, and M$ Camera up is on top:


	return a.exec();
}
