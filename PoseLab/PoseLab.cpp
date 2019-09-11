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

#include <PoseEstimation/StopWatch.h>
#include <PoseEstimation/CocoOpenCVRenderer.h>

#include "CameraVideoFrameSource.h"
#include "MovieVideoFrameSource.h"

#include "OverlayPainter.h"
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
		surfacePixels.setText(surfacePixels.placeHolder.arg(QString::number(frame.width()), QString::number(frame.height())));
		input.resize(cv::Size(frame.width() / inferencePxResizeFactor, frame.height() / inferencePxResizeFactor));
		inferencePixels.setText(inferencePixels.placeHolder.arg(QString::number(input.view.cols), QString::number(input.view.rows)));
		inferenceUpscale.setText(inferenceUpscale.placeHolder.arg(inferenceUpscaleFactor));
		inferenceConvolutionSize.setText(inferenceConvolutionSize.placeHolder.arg(inferenceConvolutionSizeValue));

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
		StopWatch stopWatch;
		vector<coco::Human> humans = pose_estimator->inference(input.tensor, inferenceUpscaleFactor);
		inferenceDuration.setText(inferenceDuration.placeHolder.arg(QString::number(stopWatch.get().count(), 'f', 0)));

		coco::OpenCvRenderer(frameRef, 
			AffineTransform::identity,
			AffineTransform(Rect2f(0.0, 0.0, 1.0, 1.0), Rect(Point(0, 0), frameRef.size()))
		).draw(humans);

		processingFPS.setText(processingFPS.placeHolder.arg(QString::number(1000.0 / fps.get().count(),'f', 2)));
		fps.reset();
	})
	, inferencePxResizeFactor(1)
	, inferenceUpscaleFactor(4)
	, inferenceResolutionGroup(this)
	, inferenceUpscalenGroup(this)
	, mediaThread()
	, videoFrameSource(nullptr)

	, surfacePixels("surface px=%1x%2", OverlayElement::Top, this)
	, inferencePixels("input px=%1x%2", OverlayElement::Top, this)
	, inferenceUpscale("upscale=%1",OverlayElement::Top, this)
	, inferenceConvolutionSize("conv2D size=%1",OverlayElement::Top, this)

	, inferenceDuration("T-inf=%1ms", OverlayElement::Bottom, this)
	, postProcessingDuration(OverlayElement::Bottom, this)
	, humanPartsGenerationDuration(OverlayElement::Bottom, this)
	, processingFPS("FPS_p=%1", OverlayElement::Bottom, this)
{
	ui.setupUi(this);
	centralWidget()->setLayout(ui.mainLayout);
	// Right toolbar controls limit vertical resize,
	// TODO Make resize dpi aware and resize so that video is 16:9
	resize(1280, 720);

	pose_estimator->loadModel();
	pose_estimator->setGaussKernelSize(25);
	inferenceThread.setPriority(QThread::Priority::LowestPriority);
	inferenceThread.start();
	inferenceThread.connect(ui.video->surface, &OpenGlVideoSurface::frameArrived, &inference, &VideoFrameProcessor::process, Qt::ConnectionType::BlockingQueuedConnection);
	inference.moveToThread(&inferenceThread);

	// TODO select check button with default value (px1) - derive from tensor mat

	newToolbarButton(ui.pxValues, inferenceResolutionGroup, "1", [this]() {	px(1); })->click();
	newToolbarButton(ui.pxValues, inferenceResolutionGroup, "2", [this]() {	px(2); });
	// TODO coco parts are offset from gt - possibly px not dividable by 16
	//newToolbarButton(ui.pxValues, inferenceResolutionGroup, "3", [this]() { px(3); });
	newToolbarButton(ui.pxValues, inferenceResolutionGroup, "4", [this]() {	px(4); });
	//newToolbarButton(ui.pxValues, inferenceResolutionGroup, "5", [this]() { px(5); });
	//newToolbarButton(ui.pxValues, inferenceResolutionGroup, "6", [this]() { px(6); });
	//newToolbarButton(ui.pxValues, inferenceResolutionGroup, "7", [this]() { px(7); });
	newToolbarButton(ui.pxValues, inferenceResolutionGroup, "8", [this]() { px(8); });

	newToolbarButton(ui.uValues, inferenceUpscalenGroup, "1", [this]() { u(1); });
	newToolbarButton(ui.uValues, inferenceUpscalenGroup, "2", [this]() { u(2); });
	newToolbarButton(ui.uValues, inferenceUpscalenGroup, "3", [this]() { u(3); });
	newToolbarButton(ui.uValues, inferenceUpscalenGroup, "4", [this]() { u(4); })->click();
	newToolbarButton(ui.uValues, inferenceUpscalenGroup, "5", [this]() { u(5); });
	newToolbarButton(ui.uValues, inferenceUpscalenGroup, "6", [this]() { u(6); });
	newToolbarButton(ui.uValues, inferenceUpscalenGroup, "7", [this]() { u(7); });
	newToolbarButton(ui.uValues, inferenceUpscalenGroup, "8", [this]() { u(8); });

	// TODO select check button with default value (u4) - derive from field
	
	connect(ui.gaussKernelSize, qOverload<int>(&QSpinBox::valueChanged), this, &PoseLab::setGaussKernelSize);
	ui.gaussKernelSize->setValue(15);

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

	ui.video->setOverlay(overlay);
	overlay.add(surfacePixels);
	overlay.add(inferencePixels);
	overlay.add(inferenceUpscale);
	overlay.add(inferenceConvolutionSize);

	overlay.add(inferenceDuration);
	// TODO
	// overlay.add(postProcessingDuration);
	// overlay.add(humanPartsGenerationDuration);

	// TODO model selection
	// TODO result tensor

	overlay.add(processingFPS);

	mediaThread.start();
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

extern QImage qt_imageFromVideoFrame(const QVideoFrame& f);

void PoseLab::addSource() {
	QFileDialog dialog(this, tr("Open Video Source"), "../testdata/");
	dialog.setFileMode(QFileDialog::AnyFile);
	dialog.setNameFilter(tr("Movie Files (*.mpg *.mkv *.3gp *.mp4 *.wmv *.webm)"));
	if (dialog.exec()) {
		QFileInfo source = dialog.selectedFiles()[0];
		show(source.absoluteFilePath());

		// Capture a frame later on to avoid black preview icons
		QTimer* timer = new QTimer;
		QObject::connect(timer, &QTimer::timeout, [this, source]() {
			// https://stackoverflow.com/questions/14828678/disconnecting-lambda-functions-in-qt5
			auto conn = std::make_shared<QMetaObject::Connection>();
			*conn = connect(ui.video->surface, &OpenGlVideoSurface::frameArrived, this, [this, conn, source](const QVideoFrame& frame) mutable {
				disconnect(*conn);
				QImage image = qt_imageFromVideoFrame(frame);
				QIcon icon(QPixmap::fromImage(image));
				QPushButton* button = new QPushButton(icon, nullptr, nullptr);
				button->setObjectName(source.absoluteFilePath());
				button->setIconSize(QSize(64, 64));
				button->setToolTip(source.baseName());
				ui.movieButtons->addWidget(button);
				connect(button, &QPushButton::pressed, this, &PoseLab::movieButtonPressed);
				}, Qt::ConnectionType::BlockingQueuedConnection);
			// TODO Resolve necessity to use BlockingConnection by making QVideoFrame& in signal declaration const
			// https://stackoverflow.com/questions/17083379/qobjectconnect-cannot-queue-arguments-of-type-int
		});
		connect(timer, &QTimer::timeout, timer, &QTimer::deleteLater);
		timer->setSingleShot(true);
		timer->start(2000);
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

void PoseLab::u(int factor) {
	inferenceUpscaleFactor = factor;
}

QToolButton* PoseLab::newToolbarButton(QLayout * layout, QButtonGroup& group, const QString& text, const std::function<void()>& pressed) {
	QToolButton* button = new QToolButton(this);
	button->setText(text);
	button->setFont(ui.centralWidget->window()->font());
	button->setCheckable(true);
	group.addButton(button);
	layout->addWidget(button);
	connect(button, &QToolButton::pressed, this, pressed);
	return button;
}

void PoseLab::setGaussKernelSize(int newSize) {
	inferenceConvolutionSizeValue = newSize;
	pose_estimator->setGaussKernelSize(newSize);
}
