#pragma once

#include <algorithm>

#include <opencv2/opencv.hpp>

#include <QThread>

#include "OpenGlVideoSurface.h"
#include "OpenGlVideoView.h"
#include "VideoFrameProcessor.h"

#include "ui_PoseLab.h"

#include <PoseEstimation/PoseEstimator.h>
#include <PoseEstimation/TensorMat.h>


class PoseLab : public QMainWindow
{
 	Q_OBJECT

public:
	PoseLab(QWidget* parent = Q_NULLPTR);
	virtual ~PoseLab();

	OpenGlVideoView* video;
	QListWidget* cameras;
	QListWidget* movies;

protected:
	std::unique_ptr<PoseEstimator> pose_estimator;
	TensorMat input;
	QThread worker;
	VideoFrameProcessor inference;

private:
	Ui::PoseLabClass ui;
};
