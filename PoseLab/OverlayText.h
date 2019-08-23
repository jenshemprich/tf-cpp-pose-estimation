#pragma once

#include "OverlayElement.h"

class OverlayText : public OverlayElement
{
	Q_OBJECT

public:
	OverlayText(Alignment alignment, QObject *parent);
	~OverlayText();

	void setText(const QString& text);

	virtual QSize size(QPainter& painter, const QRect& bounds) override;
	virtual void paint(QPainter& painter, const QRect& rect) override;

private:
	QString text;
};
