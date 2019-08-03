#pragma once

#include <QtWidgets/QMainWindow>

#include "PoseLabVideoSurface.h"

#include "ui_PoseLab.h"


class PoseLab : public QMainWindow
{
 	Q_OBJECT

public:
	PoseLab(QWidget *parent = Q_NULLPTR);
	PoseLabVideoSurface* const videoSurface;

private:
	Ui::PoseLabClass ui;
};
