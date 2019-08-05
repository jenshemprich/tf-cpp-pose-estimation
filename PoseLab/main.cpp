#include "pch.h"

#include <QtWidgets/QApplication>
#include <QCamera>
#include <qfile.h>
#include <QVideoSurfaceFormat>
#include <QCameraInfo>
#include <QVideoWidget>

#include "PoseLab.h"

using namespace std;

void setStyle(const char* resource_path) {
	QFile f(resource_path);
	if (!f.exists()) {
		qDebug() << "Unable to set stylesheet, file not found";
		exit(1);
	} else {
		f.open(QFile::ReadOnly | QFile::Text);
		QTextStream ts(&f);
		qApp->setStyleSheet(ts.readAll());
	}
}

int main(int argc, char *argv[]) {

	QApplication a(argc, argv);
	setStyle(":qdarkstyle/style.qss");

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

	// Camera stream performance 1080p@30:
	// 50-60% with OpenCV when streaming HD from the back camera 
	// 42% cpu with drawing image to label
	// 33% with QVideoView
	// Using OpenGLWidget with Vertex and fragmant shader:
	// - cpu to 28%, gpu to 43% which is quite a lot

	// TODO M$ Camera app has 3% cpu + 8% GPU wich is what we want -> WMF?
	// - M$ camera app uses GPU video module which is a little different from 3D

	// Okay, better than OpenCV but with high GPU load, and M$ Camera up is on top:

	camera->setCaptureMode(QCamera::CaptureVideo);

	// Must create new object and set that in order to apply settings
	QCameraViewfinderSettings settings(camera->viewfinderSettings());
	settings.setResolution(640, 480);
	settings.setMinimumFrameRate(15); // Wrong values are fatal
	settings.setMaximumFrameRate(15); // Wrong values are fatal
	camera->setViewfinderSettings(settings);

	camera->setViewfinder(poseLab.video->surface);
	camera->start();

	return a.exec();
}
