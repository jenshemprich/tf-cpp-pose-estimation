#pragma once

#include "OpenGlVideoView.h"

#include "ui_PoseLab.h"


class PoseLab : public QMainWindow
{
 	Q_OBJECT

public:
	PoseLab(QWidget* parent = Q_NULLPTR);
	OpenGlVideoView* video;
	QVideoWidget* videoWidget;
private:
	Ui::PoseLabClass ui;
};
