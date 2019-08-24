#pragma once

#include <algorithm>

#include <opencv2/opencv.hpp>

#include <QFile>
#include <QThread>
#include <QButtonGroup>
#include <QToolButton>

#include <PoseEstimation/PoseEstimator.h>
#include <PoseEstimation/StopWatch.h>
#include <PoseEstimation/TensorMat.h>

#include "OverlayPainter.h"
#include "OverlayText.h"
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

	void setGaussKernelSize(int newSize);

protected:
	void closeEvent(QCloseEvent* event) override;

protected:
	void show(const QString& path);
	void show(AbstractVideoFrameSource* videoFrameSource);
	AbstractVideoFrameSource* videoFrameSource;
	QThread mediaThread;

	QToolButton* newToolbarButton(QLayout * layout, QButtonGroup& group, const QString& text, const std::function<void()>& pressed);

	// TODO Split up into model-view to reduce class complexity
	// -> use slots & signals for data proapagation from inference/ui to overlay view

	std::unique_ptr<PoseEstimator> pose_estimator;
	TensorMat input;
	QThread inferenceThread;

	VideoFrameProcessor inference;
	int inferencePxResizeFactor;
	int inferenceUpscaleFactor;
	int inferenceConvolutionSizeValue;
	void px(int factor);
	void u(int factor);
	StopWatch fps;

	OverlayPainter overlay;

	OverlayText surfacePixels;
	OverlayText inferencePixels;
	OverlayText inferenceUpscale;
	OverlayText inferenceConvolutionSize;

	OverlayText inferenceDuration;
	OverlayText postProcessingDuration;
	OverlayText humanPartsGenerationDuration;

	OverlayText processingFPS;

private:
	Ui::PoseLabClass ui;
};
