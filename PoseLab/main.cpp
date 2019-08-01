#include "pch.h"

#include <QtWidgets/QApplication>

#include "PoseLab.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	PoseLab w;
	w.show();
	return a.exec();
}
