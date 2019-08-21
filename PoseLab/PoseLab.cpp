#include "pch.h"

#include <QVideoWidget>
#include <QResizeEVent>
#include <QSizePolicy>
#include <QThread>
#include <QButtonGroup>
#include <QCameraInfo>
#include <QPushButton>
#include <QFile>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QSpinBox>
#include <QList>

#include <PoseEstimation/CocoOpenCVRenderer.h>

#include "CameraVideoFrameSource.h"
#include "MovieVideoFrameSource.h"

#include "OpenCVVideoCaptureSource.h"
#include "OpenCVFFmpegSource.h"

#include "PoseLab.h"

using namespace cv;
using namespace std;

PoseLab::PoseLab(QWidget *parent)
	: QMainWindow(parent)
	, pose_estimator(unique_ptr<PoseEstimator>(new PoseEstimator("models/graph/mobilenet_thin/graph_opt.pb")))
	, input(TensorMat::AutoResize, cv::Size(0, 0))
	, inferenceThread()
	, inference([this](QVideoFrame& frame) {
		input.resize(cv::Size(frame.width() / inferencePxResizeFactor, frame.height() / inferencePxResizeFactor));

		// TODO convert directly to CV_32FC3 in TensorMat
		Mat frameRef;
		cv::Mat image;
		QVideoFrame::PixelFormat pixelFormat = frame.pixelFormat();
		if (pixelFormat == QVideoFrame::PixelFormat::Format_RGB32) {
			frameRef = Mat(frame.height(), frame.width(), CV_8UC4, frame.bits(), frame.bytesPerLine());
			cv::cvtColor(frameRef, image, cv::COLOR_BGRA2BGR);
		} else if (pixelFormat == QVideoFrame::PixelFormat::Format_BGR24) {
			frameRef = image = Mat(frame.height(), frame.width(), CV_8UC3, frame.bits(), frame.bytesPerLine());
		} else {
			assert(false);
		}

		input.copyFrom(image);
		vector<coco::Human> humans = pose_estimator->inference(input.tensor, inferenceUpscaleFactor);
		coco::OpenCvRenderer(frameRef, 
			AffineTransform::identity,
			AffineTransform(Rect2f(0.0, 0.0, 1.0, 1.0), Rect(Point(0, 0), frameRef.size()))
		).draw(humans);
	})
	, inferencePxResizeFactor(1)
	, inferenceUpscaleFactor(4)
	, inferenceResolutionGroup(this)
	, inferenceUpscalenGroup(this)
	, mediaThread()
	, videoFrameSource(nullptr)
{
	ui.setupUi(this);
	centralWidget()->setLayout(ui.mainLayout);

	pose_estimator->loadModel();
	pose_estimator->setGaussKernelSize(25);
	inferenceThread.setPriority(QThread::Priority::LowestPriority);
	inferenceThread.start();
	inferenceThread.connect(ui.video->surface, &OpenGlVideoSurface::frameArrived, &inference, &VideoFrameProcessor::process, Qt::ConnectionType::BlockingQueuedConnection);
	inference.moveToThread(&inferenceThread);

	mediaThread.start();

	inferenceResolutionGroup.addButton(ui.px1);
	inferenceResolutionGroup.addButton(ui.px2);
	inferenceResolutionGroup.addButton(ui.px4);
	inferenceResolutionGroup.addButton(ui.px8);

	// TODO select check button with default value (px1) - derive from tensor mat
	connect(ui.px1, &QToolButton::clicked, this, &PoseLab::px1);
	connect(ui.px2, &QToolButton::clicked, this, &PoseLab::px2);
	connect(ui.px4, &QToolButton::clicked, this, &PoseLab::px4);
	connect(ui.px8, &QToolButton::clicked, this, &PoseLab::px8);

	inferenceUpscalenGroup.addButton(ui.u1);
	inferenceUpscalenGroup.addButton(ui.u2);
	inferenceUpscalenGroup.addButton(ui.u4);
	inferenceUpscalenGroup.addButton(ui.u8);

	// TODO select check button with default value (u4) - derive from field
	connect(ui.u1, &QToolButton::clicked, this, &PoseLab::u1);
	connect(ui.u2, &QToolButton::clicked, this, &PoseLab::u2);
	connect(ui.u4, &QToolButton::clicked, this, &PoseLab::u4);
	connect(ui.u8, &QToolButton::clicked, this, &PoseLab::u8);

	connect(ui.gaussKernelSize, qOverload<int>(&QSpinBox::valueChanged), this, &PoseLab::setGaussKernelSize);

	QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
	foreach(const QCameraInfo & cameraInfo, cameras) {
		QString description = cameraInfo.description();

		QIcon icon; // = ":Resources/Devices/" + description + ".png";
		if (icon.isNull()) {
			icon = QIcon(":Resources/Devices/webcam.png");
		}

		if (description.indexOf("Microsoft ") == 0) {
			description = description.mid(10);
		}
		if (description.indexOf("Camera ") == 0) {
			description = description.mid(7);
		}

		QPushButton* button = new QPushButton(icon, nullptr, nullptr);
		button->setObjectName(cameraInfo.deviceName());
		button->setIconSize(QSize(64, 64));
		button->setToolTip(description);
		ui.cameraButtons->addWidget(button);
		connect(button, &QPushButton::pressed, this, &PoseLab::cameraButtonPressed);
	}

	connect(ui.addSource, &QToolButton::pressed, this, &PoseLab::addSource);

	// Workaround QLabel transparency gltch in QDarkStyle -> with alpha == 0 the color doesn't matter
	// https://stackoverflow.com/questions/9952553/transpaprent-qlabel
	// TODO report issue @ https://github.com/ColinDuquesnoy/QDarkStyleSheet
	ui.overlayTest->setStyleSheet("background-color: rgba(0,0,0,0%)");
}

void PoseLab::closeEvent(QCloseEvent* event) {
	event->accept();

	emit aboutToClose();
}

PoseLab::~PoseLab() {
	inferenceThread.disconnect(ui.video->surface, &OpenGlVideoSurface::frameArrived, &inference, &VideoFrameProcessor::process);
	inferenceThread.quit();
	inferenceThread.wait();

	if (videoFrameSource != nullptr) {
		videoFrameSource->end();
		// TODO videoFrameSource->presentFrame is called once more after calling end() and stopping the timer -> resolve and delete object here
	}

	if (videoFrameSource != nullptr) {
		delete videoFrameSource;
		videoFrameSource = nullptr;
	}

	mediaThread.quit();
	while (!mediaThread.isFinished()) {
		QApplication::processEvents();
	}

}

void PoseLab::show() {
	QMainWindow::show();
}

void PoseLab::cameraButtonPressed() {
 	QString deviceName = sender()->objectName();
	int i = 0;
	foreach(const QCameraInfo & cameraInfo, QCameraInfo::availableCameras()) {
		if (cameraInfo.deviceName() == deviceName) {
			// CameraVideoFrameSource* video = new CameraVideoFrameSource(cameraInfo.deviceName(), mediaThread);
			OpenCVVideoCaptureSource* video = new OpenCVVideoCaptureSource(QString::number(i), mediaThread);
			show(video);
			break;
		} else {
			i++;
		}
	}
}

void PoseLab::movieButtonPressed() {
	QString path = sender()->objectName();
	show(path);
}

void PoseLab::addSource() {
	QFileDialog dialog(this, tr("Open Video Source"), "../testdata/");
	dialog.setFileMode(QFileDialog::AnyFile);
	dialog.setNameFilter(tr("Movie Files (*.mpg *.mkv *.3gp *.mp4 *.wmv)"));
	if (dialog.exec()) {
		QFileInfo source = dialog.selectedFiles()[0];
		QIcon icon = QFileIconProvider().icon(source);
		QPushButton* button = new QPushButton(icon, nullptr, nullptr);
		button->setObjectName(source.absoluteFilePath());
		button->setIconSize(QSize(64, 64));
		button->setToolTip(source.baseName());
		ui.movieButtons->addWidget(button);
		connect(button, &QPushButton::pressed, this, &PoseLab::movieButtonPressed);
		show(source.absoluteFilePath());
	}
}

void PoseLab::show(const QString& path) {
	// MovieVideoFrameSource* video = new MovieVideoFrameSource(mediaThread);
	OpenCVFFMpegSource* video = new OpenCVFFMpegSource(mediaThread);
	video->setPath(path);
	show(video);
}

void PoseLab::show(AbstractVideoFrameSource* source) {
	if (videoFrameSource != nullptr) {
		videoFrameSource->end();
		videoFrameSource->deleteLater();
	}

	videoFrameSource = source;
	videoFrameSource->setTarget(ui.video->surface);
	videoFrameSource->start();
}

void PoseLab::px(int factor) {
	inferencePxResizeFactor = factor;
}

void PoseLab::px1() { px(1); }
void PoseLab::px2() { px(2); }
void PoseLab::px4() { px(4); }
void PoseLab::px8() { px(8); }

void PoseLab::u(int factor) {
	inferenceUpscaleFactor = factor;
}

void PoseLab::u1() { u(1); }
void PoseLab::u2() { u(2); }
void PoseLab::u4() { u(4); }
void PoseLab::u8() { u(8); }

void PoseLab::setGaussKernelSize(int newSize) {
	pose_estimator->setGaussKernelSize(newSize);
}


