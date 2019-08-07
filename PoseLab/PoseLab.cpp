#include "pch.h"

#include <QVideoWidget>
#include <QResizeEVent>
#include <QSizePolicy>

#include "PoseLab.h"

using namespace std;

PoseLab::PoseLab(QWidget *parent)
	: QMainWindow(parent)
	, video(nullptr)
{
	ui.setupUi(this);
	centralWidget()->setLayout(ui.gridLayout);

	video = ui.openGLvideo;
	cameras = ui.cameras;
	
	// TODO Move to OpenGLVideo constructor, but doesn't have any effect there
	QSizePolicy policy = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	policy.setHeightForWidth(true);
	policy.setWidthForHeight(true);
	video->setSizePolicy(policy);

	// Workaround QLabel transparency gltch in QDarkStyle -> with alpha == 0 the color doesn't matter
	// https://stackoverflow.com/questions/9952553/transpaprent-qlabel
	// TODO report issue @ https://github.com/ColinDuquesnoy/QDarkStyleSheet
	ui.overlayTest->setStyleSheet("background-color: rgba(255,0,0,0%)");
}
