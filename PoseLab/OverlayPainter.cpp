#include "pch.h"

#include "OverlayPainter.h"


OverlayPainter::OverlayPainter() : QPainter() {
}

OverlayPainter::~OverlayPainter() {
}

void OverlayPainter::add(OverlayElement& overlayElement) {
	if (overlayElement.alignment == OverlayElement::Top) {
		top.push_back(&overlayElement);
	}
	else {
		bottom.push_back(&overlayElement);
	}
}

void OverlayPainter::paint() {
	int width = window().width();
	int height = window().height();

	int glassHeight = 40;
	int fontSize = 18;

	QColor smokeGlass(0, 0, 0, 192);
	QBrush overlay(smokeGlass);

	QRect topArea(0, 0, width, glassHeight);
	fillRect(topArea, overlay);

	// TODO centered distribution layout of elements
	int x = 0, fill = 30;
	for_each(top.begin(), top.end(), [&](OverlayElement* element) {
		QSize size = element->size(*this, topArea);
		QRect rect(QPoint(glassHeight / 2 + x, glassHeight - fontSize / 2), size);
		element->paint(*this, rect);
		x += size.width() + fill;
	});

	QRect bottomArea(0, height - glassHeight, width, height);
	fillRect(bottomArea, overlay);
	x = 0;
	for_each(bottom.begin(), bottom.end(), [&](OverlayElement* element) {
		QSize size = element->size(*this, bottomArea);
		QRect rect(QPoint(glassHeight / 2 + x, height - fontSize / 2), size);
		element->paint(*this, rect);
		x += size.width() + fill;
	});
}
