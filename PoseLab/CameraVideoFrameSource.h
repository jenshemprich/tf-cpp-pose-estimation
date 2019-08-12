#pragma once

#include <QCameraInfo>
#include <QCamera>

#include "AbstractVideoFrameSource.h"

class CameraVideoFrameSource : public AbstractVideoFrameSource {
public:
	CameraVideoFrameSource(const QCameraInfo& cameraInfo, QObject* parent);
	virtual ~CameraVideoFrameSource();

	virtual void startWork() override;
	virtual void endWork() override;

private:
	const QCameraInfo cameraInfo;
	std::unique_ptr<QCamera> camera;
};
