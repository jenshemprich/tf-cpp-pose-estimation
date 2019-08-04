#include "pch.h"

#include <QVideoWidget>

#include "PoseLab.h"

using namespace std;

PoseLab::PoseLab(QWidget *parent)
	: QMainWindow(parent)
	, videoSurface(new PoseLabVideoSurface(this))
	, openGLvideoView(nullptr)
{
	ui.setupUi(this);
	videoSurface->setTarget(ui.videoOutput);

	videoWidget = make_unique<QVideoWidget>(new QVideoWidget(ui.videoWidgetContainer));
	videoWidget->setFixedSize(1920, 1080);

	openGLvideoView = ui.openGLvideo;
}
