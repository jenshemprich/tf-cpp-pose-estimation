#include "pch.h"

#include <QVideoWidget>
#include <QResizeEVent>
#include <QSizePolicy>
#include <QThread>
#include <QButtonGroup>
#include <QCameraInfo>
#include <QListWidget>
#include <QPushButton>
#include <QFile>
#include <QFileDialog>
#include <QMimeDatabase>
#include <QMimeType>
#include <QFileIconProvider>
#include <QScroller>
#include <QSpinBox>
#include <QList>

#include <PoseEstimation/CocoOpenCVRenderer.h>

#include "PoseLab.h"

using namespace cv;
using namespace std;

PoseLab::PoseLab(QWidget *parent)
	: QMainWindow(parent)
	, pose_estimator(unique_ptr<PoseEstimator>(new PoseEstimator("models/graph/mobilenet_thin/graph_opt.pb")))
	, input(TensorMat::AutoResize, cv::Size(0, 0))
	, worker()
	, inference([this](QVideoFrame& frame) {
		input.resize(cv::Size(frame.width() / inferencePxResizeFactor, frame.height() / inferencePxResizeFactor));
		// TODO convert directly to CV_32FC3 in TensorMat
		Mat frameRef(frame.height(), frame.width(), CV_8UC4, frame.bits(), frame.bytesPerLine());
		cv::Mat  image;
		cv::cvtColor(frameRef, image, cv::COLOR_BGRA2BGR);

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
	, videoFrameSource(nullptr)
{
	ui.setupUi(this);
	centralWidget()->setLayout(ui.gridLayout);

	pose_estimator->loadModel();
	pose_estimator->setGaussKernelSize(25);
	worker.setPriority(QThread::Priority::LowestPriority);
	worker.start();
	worker.connect(ui.openGLvideo->surface, &OpenGlVideoSurface::frameArrived, &inference, &VideoFrameProcessor::process, Qt::ConnectionType::BlockingQueuedConnection);
	inference.moveToThread(&worker);

	// TODO Move to OpenGLVideo constructor, but doesn't have any effect there
	QSizePolicy policy = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	policy.setHeightForWidth(true);
	policy.setWidthForHeight(true);
	ui.openGLvideo->setSizePolicy(policy);

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

		QListWidgetItem* item = new QListWidgetItem(icon, description, ui.cameras);
		item->setData(Qt::UserRole, QVariant(cameraInfo.deviceName()));
	}
	ui.cameras->setVisible(cameras.size() > 0);
	ui.cameras->setViewMode(cameras.size() == 1 ? QListView::ViewMode::IconMode : QListView::ViewMode::IconMode);
	ui.cameras->setStyleSheet("background-color: rgba(0,0,0,0%)");

	// TODO replace hardcoded listview width with automatic layout
	ui.cameras->setFixedWidth(64 + 8);
	ui.cameras->setFixedHeight((
		ui.cameras->iconSize().height() +
		(cameras.size() > 1 ? ui.cameras->fontInfo().pixelSize() : 0) +
		12 + 2) * cameras.size());
	connect(ui.cameras, &QListWidget::currentItemChanged, this, &PoseLab::currentCameraChanged);

	connect(ui.movies, &QListWidget::currentItemChanged, this, &PoseLab::currentMovieChanged);

	ui.movies->setFixedWidth(64 + 8);
	QScroller::grabGesture(ui.movies, QScroller::LeftMouseButtonGesture);
	// single add source
	ui.movies->setVisible(false);
	ui.movies->setStyleSheet("background-color: rgba(0,0,0,0%)");

	connect(ui.addSource, &QToolButton::pressed, this, &PoseLab::addSource);

	//if (QFile::exists("../testdata")) {
	//	showMovieFolder("../testdata");
	//}

	// TODO Deprecated -> remove related code
	connect(ui.openMovieFolder, &QPushButton::pressed, this, &PoseLab::selectMovieFolder);
	ui.openMovieFolder->setVisible(false);

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
	worker.quit();
	worker.wait();
}

void PoseLab::currentCameraChanged(QListWidgetItem* current, QListWidgetItem* previous) {

}

void PoseLab::currentMovieChanged(QListWidgetItem* current, QListWidgetItem* previous) {
	if (current) {
		VideoFrameSource* video = new VideoFrameSource(nullptr);
		video->setPath(current->data(Qt::UserRole).toString());
		showSource(video);
	}
}

void PoseLab::selectMovieFolder() {
	QFileDialog dialog(this, tr("Open Movie Folder"), "../testdata/");
	dialog.setFileMode(QFileDialog::AnyFile);
	dialog.setNameFilter(tr("Movie Files (*.mpg *.mkv *.3gp *.mp4 *.wmv)"));
	if (dialog.exec()) {
		QDir dir = QDir(dialog.selectedFiles()[0]);
		dir.cdUp();
		showMovieFolder(dir.absolutePath());
	}
}

void PoseLab::addSource() {
	QFileDialog dialog(this, tr("Open Video Source"), "../testdata/");
	dialog.setFileMode(QFileDialog::AnyFile);
	dialog.setNameFilter(tr("Movie Files (*.mpg *.mkv *.3gp *.mp4 *.wmv)"));
	if (dialog.exec()) {
		QFileInfo source = dialog.selectedFiles()[0];
		QIcon icon = QFileIconProvider().icon(source);
		QListWidgetItem* item = new QListWidgetItem(icon, nullptr, ui.movies);
		item->setToolTip(source.baseName());
		item->setData(Qt::UserRole, QVariant(source.absoluteFilePath()));
		ui.movies->setVisible(true);
		ui.movies->setItemSelected(item, true);
		// TODO Learn why connected signal isn't emmited
		currentMovieChanged(item, nullptr);
	}
}

void PoseLab::showMovieFolder(const QString& folder) {
	ui.movies->clear();
	QDir dir = QDir(folder);
	dir.setNameFilters(QStringList() << "*.mpg" << "*.mkv" << "*.3gp" << "*.mp4" << "*.wmv");
	QFileInfoList list = dir.entryInfoList();
	for (int i = 0; i < list.size(); ++i) {
		QFileInfo fileInfo = list.at(i);
		QIcon icon = QFileIconProvider().icon(fileInfo);
		QListWidgetItem* item = new QListWidgetItem(icon, fileInfo.baseName().replace("_"," "), ui.movies);
		item->setToolTip(fileInfo.baseName());
		item->setData(Qt::UserRole, QVariant(fileInfo.absoluteFilePath()));
	}
}

void PoseLab::showSource(VideoFrameSource* source) {
	if (videoFrameSource.get() != nullptr) {
		videoFrameSource->end();
	}

	videoFrameSource = unique_ptr<VideoFrameSource>(source);
	connect(this, &PoseLab::aboutToClose, videoFrameSource.get(), &VideoFrameSource::end);
	videoFrameSource->setTarget(ui.openGLvideo->surface);
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


