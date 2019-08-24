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

QSize OverlayText::size(QPainter& painter, const QRect& bounds) {
	lock_guard<mutex> lock(data);

	// TODO Code duplication
	// TODO layout decisions distributed between overlay text and painter
	QFont f = painter.font();
	int fontHeight = (bounds.height() - 4) / 2;
	f.setPointSize(fontHeight);
	painter.setFont(f);
	return QSize(painter.fontMetrics().horizontalAdvance(text), bounds.height());
}

void OverlayText::paint(QPainter& painter, const QRect& bounds) {
	lock_guard<mutex> lock(data);

	QFont f = painter.font();
	int fontHeight = bounds.height() - 22;
	f.setPointSize(fontHeight);
	painter.setFont(f);
	painter.drawText(QPoint(bounds.left(), bounds.top() + bounds.height() / 4 * 3), text);
}
