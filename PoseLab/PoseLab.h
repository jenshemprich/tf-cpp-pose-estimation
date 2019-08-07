#pragma once

#include <opencv2/opencv.hpp>

#include "OpenGlVideoView.h"

#include "ui_PoseLab.h"

#include <PoseEstimation/PoseEstimator.h>
#include <PoseEstimation/TensorMat.h>

class PoseLab : public QMainWindow
{
 	Q_OBJECT

public:
	PoseLab(QWidget* parent = Q_NULLPTR);
	OpenGlVideoView* video;
	QListWidget* cameras;
	QListWidget* movies;

protected:
	void inference(QVideoFrame& frame);
	std::unique_ptr<PoseEstimator> pose_estimator;
	TensorMat input;

	QImage frameToImage(QVideoFrame& frame);
	cv::Mat QImageToCvMat(const QImage& inImage, bool inCloneImageData);
	cv::Mat QPixmapToCvMat(const QPixmap& inPixmap, bool inCloneImageData);

private:
	Ui::PoseLabClass ui;
};
