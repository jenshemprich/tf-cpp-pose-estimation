#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_PoseLab.h"

#include <opencv2/opencv.hpp>
#include <opencv2/core/base.hpp>
#include <opencv2/core/mat.hpp>

class PoseLab : public QMainWindow
{
// 	Q_OBJECT

public:
	PoseLab(QWidget *parent = Q_NULLPTR);

	void updateImage(cv::Mat* image);
private:
	Ui::PoseLabClass ui;
};
