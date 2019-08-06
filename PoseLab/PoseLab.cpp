#include "pch.h"

#include <QVideoWidget>
#include <QResizeEVent>

#include "PoseLab.h"

using namespace std;

PoseLab::PoseLab(QWidget *parent)
	: QMainWindow(parent)
	, video(nullptr)
{
	ui.setupUi(this);
	centralWidget()->setLayout(ui.gridLayout);

	video = ui.openGLvideo;
	videoWidget = ui.videoWidget;

	// Workaround gltch in QDarkStyle -> with alpha == 0 the color doesn't matter
	// https://stackoverflow.com/questions/9952553/transpaprent-qlabel
	// TODO report issue @ https://github.com/ColinDuquesnoy/QDarkStyleSheet
	ui.overlayTest->setStyleSheet("background-color: rgba(255,0,0,0%)");
}
