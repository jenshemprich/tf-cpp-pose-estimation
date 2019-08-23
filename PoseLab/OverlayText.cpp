#include "pch.h"

#include "OverlayText.h"

OverlayText::OverlayText(Alignment alignment, QObject *parent)
	: OverlayElement(alignment, parent)
{}

OverlayText::~OverlayText() {
}

void OverlayText::setText(const QString& text) {
	this->text = text;
}

int fontSize = 18;

QSize OverlayText::size(QPainter& painter, const QRect& bounds) {
	// TODO Code duplication
	QFont f = painter.font();
	f.setPointSize(fontSize);
	painter.setFont(f);

	return QSize(painter.fontMetrics().horizontalAdvance(text), fontSize);
}

void OverlayText::paint(QPainter& painter, const QRect& rect) {
	QFont f = painter.font();
	f.setPointSize(fontSize);
	painter.setFont(f);
	painter.drawText(rect.topLeft(), text);
}
