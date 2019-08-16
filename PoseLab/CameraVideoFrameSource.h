#pragma once

#include <QCameraInfo>
#include <QCamera>

#include "AbstractVideoFrameSource.h"

class CameraVideoFrameSource : public AbstractVideoFrameSource {
public:
	CameraVideoFrameSource(const QString& deviceName, QThread& worker);
	virtual ~CameraVideoFrameSource();

	virtual void startWork() override;
	virtual void endWork() override;

private:
	const QString deviceName;
	QCamera* camera;
};
