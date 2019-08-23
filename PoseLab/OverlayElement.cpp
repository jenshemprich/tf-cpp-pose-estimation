#include "pch.h"
#include "OverlayElement.h"

OverlayElement::OverlayElement(Alignment alignment, QObject *parent)
	: QObject(parent)
	, alignment(alignment)
{}

OverlayElement::~OverlayElement()
{}

