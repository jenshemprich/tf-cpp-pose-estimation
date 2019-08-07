#include "pch.h"

#include <QVideoWidget>
#include <QResizeEVent>
#include <QSizePolicy>

#include <PoseEstimation/CocoOpenCVRenderer.h>

#include "PoseLab.h"

using namespace cv;
using namespace std;

PoseLab::PoseLab(QWidget *parent)
	: QMainWindow(parent)
	, video(nullptr)
	, pose_estimator(unique_ptr<PoseEstimator>(new PoseEstimator("models/graph/mobilenet_thin/graph_opt.pb")))
	, input(TensorMat::AutoResize, cv::Size(0, 0))
{
	ui.setupUi(this);
	centralWidget()->setLayout(ui.gridLayout);

	video = ui.openGLvideo;

	pose_estimator->loadModel();
	pose_estimator->setGaussKernelSize(25);

	connect(video, &OpenGlVideoView::frameArrived, this, &PoseLab::inference);


	cameras = ui.cameras;
	movies = ui.movies;

	// TODO Move to OpenGLVideo constructor, but doesn't have any effect there
	QSizePolicy policy = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	policy.setHeightForWidth(true);
	policy.setWidthForHeight(true);
	video->setSizePolicy(policy);

	// Workaround QLabel transparency gltch in QDarkStyle -> with alpha == 0 the color doesn't matter
	// https://stackoverflow.com/questions/9952553/transpaprent-qlabel
	// TODO report issue @ https://github.com/ColinDuquesnoy/QDarkStyleSheet
	ui.overlayTest->setStyleSheet("background-color: rgba(255,0,0,0%)");
}

void PoseLab::inference(QVideoFrame& frame) {
	// TODO convert directly to CV_32FC3 in TensorMat
	cv::Mat  frameRef(frame.height(), frame.width(), CV_8UC4, frame.bits(), frame.bytesPerLine());
	cv::Mat  image;
	cv::cvtColor(frameRef, image, cv::COLOR_BGRA2BGR);

	input.copyFrom(image);
	vector<coco::Human> humans = pose_estimator->inference(input.tensor, 4);

	const Rect roi = Rect(Point(0,0), frameRef.size());
	AffineTransform view(Rect2f(0.0, 0.0, 1.0, 1.0), roi);

	coco::OpenCvRenderer(frameRef, AffineTransform::identity, view).draw(humans);
}
