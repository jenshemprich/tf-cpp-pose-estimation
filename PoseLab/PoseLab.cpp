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
}
