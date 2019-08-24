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

	// TODO Same size for all overlays -> compute all sizes first, use minimum
	paint(top, QRect(0, 0, width, glassHeight), OverlayElement::Top);
	paint(bottom, QRect(0, height - glassHeight, width, glassHeight), OverlayElement::Bottom);
}

void OverlayPainter::paint(const std::vector<OverlayElement*>& elements, const QRect& region, OverlayElement::Alignment alignment) {
	QColor smokeGlass(0, 0, 0, 192);
	QBrush overlay(smokeGlass);

	int x = 0, fill = 30;
	int glassTop = region.top();
	int glassHeight = region.height();
	int fontSize = 18;

	do {
		x = 0;
		QRect glass = QRect(region.left(), glassTop, region.width(), glassHeight);
		for_each(elements.begin(), elements.end(), [&](OverlayElement* element) {
			QSize size = element->size(*this, glass);
			x += size.width() + fill;
			});
		x -= fill;
		if (x < region.width()) {
			fillRect(glass, overlay);
			x = (region.width() - x) / 2;
			for_each(elements.begin(), elements.end(), [&](OverlayElement* element) {
				QSize size = element->size(*this, glass);
				QRect rect(QPoint(x, glass.top()), size);
				element->paint(*this, rect);
				x += size.width() + fill;
				});
			break;
		}
		else {
			glassHeight /= 2;
			glassTop += alignment == OverlayElement::Top ? 0 : glassHeight;
		}
	} while (x > region.width() && glassHeight >= 10);
}
