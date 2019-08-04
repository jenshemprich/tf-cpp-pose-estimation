#pragma once

#include <QtWidgets/QMainWindow>
#include <QVideoWidget>

#include "OpenGlVideoView.h"
#include "PoseLabVideoSurface.h"

#include "ui_PoseLab.h"


class PoseLab : public QMainWindow
{
 	Q_OBJECT

public:
	PoseLab(QWidget *parent = Q_NULLPTR);
	PoseLabVideoSurface* const videoSurface;
	OpenGlVideoView* openGLvideoView;
	std::unique_ptr<QVideoWidget> videoWidget;
private:
	Ui::PoseLabClass ui;
};
