#include "pch.h"

#include <QVideoWidget>
#include <QResizeEVent>
#include <QSizePolicy>
#include <QThread>
#include <QButtonGroup>

#include <PoseEstimation/CocoOpenCVRenderer.h>

#include "PoseLab.h"

using namespace cv;
using namespace std;

PoseLab::PoseLab(QWidget *parent)
	: QMainWindow(parent)
	, video(nullptr)
	, pose_estimator(unique_ptr<PoseEstimator>(new PoseEstimator("models/graph/mobilenet_thin/graph_opt.pb")))
	, input(TensorMat::AutoResize, cv::Size(0, 0))
	, worker()
	, inference([this](QVideoFrame& frame) {
		// TODO convert directly to CV_32FC3 in TensorMat
		Mat frameRef(frame.height(), frame.width(), CV_8UC4, frame.bits(), frame.bytesPerLine());
		cv::Mat  image;
		cv::cvtColor(frameRef, image, cv::COLOR_BGRA2BGR);

		input.copyFrom(image);
		vector<coco::Human> humans = pose_estimator->inference(input.tensor, 4);
		coco::OpenCvRenderer(frameRef, 
			AffineTransform::identity,
			AffineTransform(Rect2f(0.0, 0.0, 1.0, 1.0), Rect(Point(0, 0), frameRef.size()))
		).draw(humans);
	})
	, inferenceResolutionGroup(this)
	, inferenceUpscalenGroup(this)
	, videoFrameSource(new VideoFrameSource(nullptr))
{
	ui.setupUi(this);
	centralWidget()->setLayout(ui.gridLayout);

	video = ui.openGLvideo;

	pose_estimator->loadModel();
	pose_estimator->setGaussKernelSize(25);

	worker.setPriority(QThread::Priority::LowestPriority);
	worker.start();
	worker.connect(video->surface, &OpenGlVideoSurface::frameArrived, &inference, &VideoFrameProcessor::process, Qt::ConnectionType::BlockingQueuedConnection);
	inference.moveToThread(&worker);

	cameras = ui.cameras;
	movies = ui.movies;

	// TODO Move to OpenGLVideo constructor, but doesn't have any effect there
	QSizePolicy policy = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	policy.setHeightForWidth(true);
	policy.setWidthForHeight(true);
	video->setSizePolicy(policy);

	videoFrameSource->setPath("../testdata/Handstand_240p.3gp");
	videoFrameSource->setTarget(video->surface);
	videoFrameSource->start();
	connect(this, &PoseLab::aboutToClose, videoFrameSource.get(), &VideoFrameSource::end);

	inferenceResolutionGroup.addButton(ui.px1);
	inferenceResolutionGroup.addButton(ui.px2);
	inferenceResolutionGroup.addButton(ui.px4);
	inferenceResolutionGroup.addButton(ui.px8);

	inferenceUpscalenGroup.addButton(ui.u1);
	inferenceUpscalenGroup.addButton(ui.u2);
	inferenceUpscalenGroup.addButton(ui.u4);
	inferenceUpscalenGroup.addButton(ui.u8);

	// Workaround QLabel transparency gltch in QDarkStyle -> with alpha == 0 the color doesn't matter
	// https://stackoverflow.com/questions/9952553/transpaprent-qlabel
	// TODO report issue @ https://github.com/ColinDuquesnoy/QDarkStyleSheet
	ui.overlayTest->setStyleSheet("background-color: rgba(255,0,0,0%)");
}

void PoseLab::closeEvent(QCloseEvent* event) {
	event->accept();
	emit aboutToClose();
}

PoseLab::~PoseLab() {
	worker.quit();
	worker.wait();
}
