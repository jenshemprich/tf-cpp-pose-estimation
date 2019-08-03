#include "pch.h"


#include "PoseLab.h"

PoseLab::PoseLab(QWidget *parent)
	: QMainWindow(parent)
	, videoSurface(new PoseLabVideoSurface(this))
{
	ui.setupUi(this);
	videoSurface->setTarget(ui.videoOutput);
}
