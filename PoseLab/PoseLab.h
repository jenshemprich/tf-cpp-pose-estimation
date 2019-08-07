#pragma once

#include "OpenGlVideoView.h"

#include "ui_PoseLab.h"


class PoseLab : public QMainWindow
{
 	Q_OBJECT

public:
	PoseLab(QWidget* parent = Q_NULLPTR);
	OpenGlVideoView* video;
	QListWidget* cameras;
	QListWidget* movies;
private:
	Ui::PoseLabClass ui;
};
