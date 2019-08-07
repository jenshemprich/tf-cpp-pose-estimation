#include "pch.h"

#include <QVideoWidget>
#include <QResizeEVent>
#include <QSizePolicy>

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
	// TODO create CV_UC4 reference and convert directly to CV_32FC3 in TensorMat
	Mat image = QImageToCvMat(frameToImage(frame), true);
	input.copyFrom(image);
	vector<coco::Human> humans = pose_estimator->inference(input.tensor, 4);
	qDebug() << humans.size() << " humans detected";
}

// TODO skip image creation and convert to cv::Mat directly
QImage PoseLab::frameToImage(QVideoFrame& frame) {
	return QImage(frame.bits(),
		frame.width(),
		frame.height(),
		frame.bytesPerLine(),
		QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat()));
}

// based on https://asmaloney.com/2013/11/code/converting-between-cvmat-and-qimage-or-qpixmap/
// If inImage exists for the lifetime of the resulting cv::Mat, pass false to inCloneImageData to share inImage's
// data with the cv::Mat directly
//    NOTE: Format_RGB888 is an exception since we need to use a local QImage and thus must clone the data regardless
//    NOTE: This does not cover all cases - it should be easy to add new ones as required.
cv::Mat PoseLab::QImageToCvMat(const QImage& inImage, bool inCloneImageData = true) {
	switch (inImage.format())
	{
		// 8-bit, 4 channel
	case QImage::Format_ARGB32:
	case QImage::Format_ARGB32_Premultiplied:
	{
		cv::Mat  mat(inImage.height(), inImage.width(),
			CV_8UC4,
			const_cast<uchar*>(inImage.bits()),
			static_cast<size_t>(inImage.bytesPerLine())
		);

		return (inCloneImageData ? mat.clone() : mat);
	}

	// 8-bit, 3 channel
	case QImage::Format_RGB32:
	{
		if (!inCloneImageData)
		{
			qWarning() << "ASM::QImageToCvMat() - Conversion requires cloning so we don't modify the original QImage data";
		}

		cv::Mat  mat(inImage.height(), inImage.width(),
			CV_8UC4,
			const_cast<uchar*>(inImage.bits()),
			static_cast<size_t>(inImage.bytesPerLine())
		);

		cv::Mat  matNoAlpha;

		cv::cvtColor(mat, matNoAlpha, cv::COLOR_BGRA2BGR);   // drop the all-white alpha channel

		return matNoAlpha;
	}

	// 8-bit, 3 channel
	case QImage::Format_RGB888:
	{
		if (!inCloneImageData)
		{
			qWarning() << "ASM::QImageToCvMat() - Conversion requires cloning so we don't modify the original QImage data";
		}

		QImage   swapped = inImage.rgbSwapped();

		return cv::Mat(swapped.height(), swapped.width(),
			CV_8UC3,
			const_cast<uchar*>(swapped.bits()),
			static_cast<size_t>(swapped.bytesPerLine())
		).clone();
	}

	// 8-bit, 1 channel
	case QImage::Format_Indexed8:
	{
		cv::Mat  mat(inImage.height(), inImage.width(),
			CV_8UC1,
			const_cast<uchar*>(inImage.bits()),
			static_cast<size_t>(inImage.bytesPerLine())
		);

		return (inCloneImageData ? mat.clone() : mat);
	}

	default:
		qWarning() << "ASM::QImageToCvMat() - QImage format not handled in switch:" << inImage.format();
		break;
	}

	return cv::Mat();
}

// If inPixmap exists for the lifetime of the resulting cv::Mat, pass false to inCloneImageData to share inPixmap's data
// with the cv::Mat directly
//    NOTE: Format_RGB888 is an exception since we need to use a local QImage and thus must clone the data regardless
cv::Mat PoseLab::QPixmapToCvMat(const QPixmap& inPixmap, bool inCloneImageData = true) {
	return QImageToCvMat(inPixmap.toImage(), inCloneImageData);
}

