#pragma once

#include <algorithm>

#include <opencv2/opencv.hpp>

#include <QFile>
#include <QThread>
#include <QButtonGroup>

#include <PoseEstimation/PoseEstimator.h>
#include <PoseEstimation/TensorMat.h>

#include "OpenGlVideoSurface.h"
#include "OpenGlVideoView.h"
#include "VideoFrameProcessor.h"
#include "AbstractVideoFrameSource.h"

#include "ui_PoseLab.h"


class PoseLab : public QMainWindow
{
 	Q_OBJECT

public:
	PoseLab(QWidget* parent = Q_NULLPTR);
	virtual ~PoseLab();

	QButtonGroup inferenceResolutionGroup;
	QButtonGroup inferenceUpscalenGroup;

	void show();

signals:
	void aboutToClose();

public slots:
	void cameraButtonPressed();
	void movieButtonPressed();
	void addSource();

	void px1();
	void px2();
	void px4();
	void px8();

	void u1();
	void u2();
	void u4();
	void u8();

	void setGaussKernelSize(int newSize);

protected:
	void closeEvent(QCloseEvent* event) override;

protected:
	void show(const QString& path);
	void show(AbstractVideoFrameSource* videoFrameSource);
	AbstractVideoFrameSource* videoFrameSource;
	QThread mediaThread;

	std::unique_ptr<PoseEstimator> pose_estimator;
	TensorMat input;
	QThread inferenceThread;

	VideoFrameProcessor inference;
	int inferencePxResizeFactor;
	int inferenceUpscaleFactor;
	void px(int factor);
	void u(int factor);

private:
	Ui::PoseLabClass ui;
};
