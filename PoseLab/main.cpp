#include "pch.h"

#include <QtWidgets/QApplication>
#include <QCamera>
#include <qfile.h>
#include <QVideoSurfaceFormat>
#include <QCameraInfo>
#include <QMediaPlayer>

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

unique_ptr<QCamera> camera() {
	QCamera* camera = nullptr;
	QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
	foreach(const QCameraInfo & cameraInfo, cameras) {
		//const QString description = cameraInfo.description();
		//if (description.contains("Front")) {
		if (cameraInfo.position() == QCamera::Position::FrontFace) {
			camera = new QCamera(cameraInfo);
		}
	}

	// TODO Cameras don't seem to have a position on Surface 4 
	// TODO Camera is alwys rear, no matter what (description says front)
	if (camera == nullptr) {
		camera = new QCamera(cameras[0]);
	}
	return unique_ptr<QCamera>(camera);
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
void show(unique_ptr<QCamera>& camera, QAbstractVideoSurface* surface) {
	camera->setCaptureMode(QCamera::CaptureVideo);

	// Must create new object and set that in order to apply settings
	//QCameraViewfinderSettings settings(camera->viewfinderSettings());
	//settings.setResolution(640, 480);
	//settings.setMinimumFrameRate(15); // Wrong values are fatal
	//settings.setMaximumFrameRate(15); // Wrong values are fatal
	//camera->setViewfinderSettings(settings);

	camera->setViewfinder(surface);
	camera->start();
}

void show(unique_ptr<QCamera>& camera, QVideoWidget* surface) {
	camera->setCaptureMode(QCamera::CaptureVideo);

	// Must create new object and set that in order to apply settings
	QCameraViewfinderSettings settings(camera->viewfinderSettings());
	settings.setResolution(640, 480);
	settings.setMinimumFrameRate(15); // Wrong values are fatal
	settings.setMaximumFrameRate(15); // Wrong values are fatal
	camera->setViewfinderSettings(settings);

	camera->setViewfinder(surface);
	camera->start();
}

unique_ptr<QMediaPlayer> mediaPlayer(const char* path) {
	QMediaPlayer* player = new QMediaPlayer();
	QUrl url = QUrl::fromLocalFile(path);
	qDebug() << url;
	player->setMedia(url);

	return unique_ptr<QMediaPlayer>(player);
}

void show(unique_ptr< QMediaPlayer >& player, QAbstractVideoSurface* surface) {
	player->setVideoOutput(surface);
	player->play();
	assert(player->isAvailable());
}

int main(int argc, char* argv[]) {
	QApplication a(argc, argv);
	setStyle(":qdarkstyle/style.qss");

	PoseLab poseLab;
	poseLab.show();

	QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
	foreach(const QCameraInfo & cameraInfo, cameras) {
		QString description = cameraInfo.description();

		QString iconResource = ":Resources/Devices/" + description + ".png";
		if (!QFile::exists(iconResource)) {
			iconResource = ":Resources/Devices/camera_lens.png";
		} 

		if (description.indexOf("Microsoft ") == 0) {
			description = description.mid(10);
		}
		if (description.indexOf("Camera ") == 0) {
			description = description.mid(7);
		}
		new QListWidgetItem(QIcon(iconResource), description, poseLab.cameras);
	}
	poseLab.cameras->setVisible(cameras.size() > 0);

	// TODO Show item text only if multiple cameras

	// TODO resolve hardcoded horizontal size hack to good practice
	poseLab.cameras->setFixedWidth(96);
	poseLab.movies->setFixedWidth(96);

	//unique_ptr<QCamera> cam = camera();
	//show(cam, poseLab.video->surface);

	//unique_ptr<QMediaPlayer> player1 = mediaPlayer("../testdata/Yoga Morning Fresh  _  Yoga With Adriene 360p.mp4");
	unique_ptr<QMediaPlayer> player1 = mediaPlayer("../testdata/Handstand_240p.3gp");
	show(player1, poseLab.video->surface);

	// https://forum.qt.io/topic/89856/switch-qmultimedia-backend-without-recompiling-whole-qt/2
	// -> install https://github.com/Nevcairiel/LAVFilters/releases or Haali Media Splitter
	// - mp4 still won't play correctly
	// wmf backend doesn't play avi, and won't enum cameras
	// TODO webm 3fp plays good -> convert samples  or use OpenCV

	return a.exec();
}
