#pragma once

#include <QObject>
#include <QPainter>

class OverlayElement : public QObject {
	Q_OBJECT

public:
	enum Alignment {
		Top,
		Bottom
	};

	OverlayElement(Alignment alignment, QObject* parent);
	virtual ~OverlayElement();

	const Alignment alignment;

	virtual QSize size(QPainter& painter, const QRect& bounds)=0;
	virtual void paint(QPainter& painter, const QRect& rect)=0;
};
