#pragma once

#include <QOpenGLWidget>
#include <QAbstractVideoSurface>

#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>

class OpenGlVideoSurface;

class OpenGlVideoView : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT

public:
	OpenGlVideoView(QWidget *parent);
	~OpenGlVideoView();

	OpenGlVideoSurface* surface;

virtual bool isValid() const {
	return (wasInitialized());
}

bool wasInitialized() const {
	return (vertexArrayObject.isCreated());
}

protected slots:
	void setFrame(QVideoFrame& frame);
	//void setFrame(QImage frame);

protected:
	void initializeGL();
	void resizeGL(int w, int h)	override;
	void paintGL() override;

	QOpenGLVertexArrayObject vertexArrayObject;
	QOpenGLBuffer quadVertexBuffer, quadIndexBuffer;
	QOpenGLShaderProgram program;
	QOpenGLTexture* videoTexture;

	int localWidth, localHeight;
	qreal devicePixelRatio;

	bool hasHeightForWidth() const override;
	int heightForWidth(int width) const override;
	int videoWidth, videoHeight;
};
