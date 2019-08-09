#pragma once

#include <algorithm>

#include <opencv2/opencv.hpp>

#include <QThread>
#include <QButtonGroup>

#include <PoseEstimation/PoseEstimator.h>
#include <PoseEstimation/TensorMat.h>

#include "OpenGlVideoSurface.h"
#include "OpenGlVideoView.h"
#include "VideoFrameProcessor.h"
#include "VideoFrameSource.h"

#include "ui_PoseLab.h"


class PoseLab : public QMainWindow
{
 	Q_OBJECT

public:
	PoseLab(QWidget* parent = Q_NULLPTR);
	virtual ~PoseLab();

	OpenGlVideoView* video;
	QListWidget* cameras;
	QListWidget* movies;

	QButtonGroup inferenceResolutionGroup;
	QButtonGroup inferenceUpscalenGroup;

	std::unique_ptr<VideoFrameSource> videoFrameSource;

signals:
	void aboutToClose();

protected:
	void closeEvent(QCloseEvent* event) override;

	std::unique_ptr<PoseEstimator> pose_estimator;
	TensorMat input;
	QThread worker;
	VideoFrameProcessor inference;

private:
	Ui::PoseLabClass ui;
};
