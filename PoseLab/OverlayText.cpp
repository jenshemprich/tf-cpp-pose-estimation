#include "pch.h"

#include "OverlayText.h"

using namespace std;

OverlayText::OverlayText(Alignment alignment, QObject *parent)
	: OverlayElement(alignment, parent)
	, placeHolder()
{}

OverlayText::OverlayText(const QString& placeHolder, Alignment alignment, QObject* parent)
	: OverlayElement(alignment, parent)
	, placeHolder(placeHolder)
{}

OverlayText::~OverlayText() {
}

void OverlayText::setText(const QString& text) {
	lock_guard<mutex> lock(data);
	this->text = text;
}

// TODO layout decisions distributed between overlay text and painter

QSize OverlayText::size(QPainter& painter, const QRect& bounds) {
	lock_guard<mutex> lock(data);
	chooseFont(painter, bounds);
	return QSize(painter.fontMetrics().horizontalAdvance(text), bounds.height());
}

void OverlayText::paint(QPainter& painter, const QRect& bounds) {
	lock_guard<mutex> lock(data);
	chooseFont(painter, bounds);
	painter.drawText(QPoint(bounds.left(), bounds.top() + bounds.height() / 4 * 3), text);
}

void OverlayText::chooseFont(QPainter& painter, const QRect& bounds) {
	QFont f = painter.font();
	int fontHeight = (bounds.height() - 8) / 1;
	f.setPixelSize(fontHeight);
	painter.setFont(f);
}
