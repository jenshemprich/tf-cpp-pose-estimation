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
#include "VideoFrameSource.h"

#include "ui_PoseLab.h"


class PoseLab : public QMainWindow
{
 	Q_OBJECT

public:
	PoseLab(QWidget* parent = Q_NULLPTR);
	virtual ~PoseLab();

	QButtonGroup inferenceResolutionGroup;
	QButtonGroup inferenceUpscalenGroup;

	std::unique_ptr<VideoFrameSource> videoFrameSource;

signals:
	void aboutToClose();

public slots:
	void currentCameraChanged(QListWidgetItem* current, QListWidgetItem* previous);
	void currentMovieChanged(QListWidgetItem* current, QListWidgetItem* previous);
	void selectMovieFolder();
	void addSource();
	void showSource(VideoFrameSource* videoFrameSource);

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

	void px(int factor);
	void u(int factor);

	std::unique_ptr<PoseEstimator> pose_estimator;
	TensorMat input;
	QThread worker;
	VideoFrameProcessor inference;
	int inferencePxResizeFactor;
	int inferenceUpscaleFactor;

	void showMovieFolder(const QString& folder);

	void setFixedHeight(QListView* listView, int itemCount);

private:
	Ui::PoseLabClass ui;
};
