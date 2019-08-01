#include "pch.h"

#include "PoseLab.h"

PoseLab::PoseLab(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
}


void PoseLab::updateImage(cv::Mat* img) {
	QPixmap pxmap = QPixmap::fromImage(QImage(img->data, img->cols, img->rows, img->step, QImage::Format_RGB888));
	QGraphicsScene* scene = ui.graphicsView->scene();
	scene->addPixmap(pxmap);

	// pxmap.
}
