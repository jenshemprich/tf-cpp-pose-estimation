#include "pch.h"

#include <QtWidgets/QApplication>
#include <QFile>

#include "PoseLab.h"

using namespace std;

void setStyle(const char* resource_path) {
	QFile f(resource_path);
	if (!f.exists()) {
		qDebug() << "Unable to set stylesheet, file not found";
		exit(1);
	} else {
		f.open(QFile::ReadOnly | QFile::Text);
		QTextStream ts(&f);
		qApp->setStyleSheet(ts.readAll());
	}
}

int main(int argc, char* argv[]) {
	QApplication a(argc, argv);
	// TODO font size propagation doesn't work in qDarkStyle
	// - when set all widget take some kind of defaul font size 
	// + with the default size, all widgets inherit the font size of the QMainWindow (PoseLabClass)
	setStyle(":qdarkstyle/style.qss");

	PoseLab poseLab;
	poseLab.show();

	// https://forum.qt.io/topic/89856/switch-qmultimedia-backend-without-recompiling-whole-qt/2
	// -> install https://github.com/Nevcairiel/LAVFilters/releases or Haali Media Splitter
	// - mp4 still won't play correctly
	// wmf backend doesn't play avi, and won't enum cameras
	// TODO webm 3fp plays good -> convert samples  or use OpenCV

	return a.exec();
}
