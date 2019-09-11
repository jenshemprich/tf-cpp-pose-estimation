#include "pch.h"

#include <vector>

#include "OverlayPainter.h"

using namespace std;

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
	const int width = window().width();
	const int height = window().height();
	const int desiredHeight = 80;

	vector<int> v;
	v.push_back(overlayHeight(top, QRect(0, 0, width, desiredHeight)));
	v.push_back(overlayHeight(bottom, QRect(0, height - desiredHeight, width, desiredHeight)));
	const int glassHeight = *std::min_element(std::begin(v), std::end(v));

	paint(top, QRect(0, 0, width, glassHeight));
	paint(bottom, QRect(0, height - glassHeight, width, glassHeight));
}

int OverlayPainter::overlayHeight(const std::vector<OverlayElement*>& elements, const QRect& region) {
	int x, fill = 30;
	int glassHeight = region.height();

	do {
		x = fill;
		QRect glass = QRect(region.left(), region.top(), region.width(), glassHeight);
		for_each(elements.begin(), elements.end(), [&](OverlayElement* element) {
			QSize size = element->size(*this, glass);
			x += size.width() + fill;
			});
//		x -= fill;
		if (x > region.width()) {
			glassHeight /= 2;
		}
	} while (x > region.width() && glassHeight >= 10);
	return glassHeight;
}

void OverlayPainter::paint(const std::vector<OverlayElement*>& elements, const QRect& region) {
	QColor smokeGlass(0, 0, 0, 192);
	QBrush overlay(smokeGlass);

	int x = 0, fill = 30;
	int glassTop = region.top();

	int widthOfElements = 0;
	for_each(elements.begin(), elements.end(), [&](OverlayElement* element) {
		QSize size = element->size(*this, region);
		widthOfElements += size.width() + fill;
	});

	fillRect(region, overlay);
	x = (region.width() - widthOfElements) / 2;
	for_each(elements.begin(), elements.end(), [&](OverlayElement* element) {
		QSize size = element->size(*this, region);
		QRect rect(QRect(region.left() + x, region.top(), region.width(), region.height()));
		element->paint(*this, rect);
		x += size.width() + fill;
	});
}
