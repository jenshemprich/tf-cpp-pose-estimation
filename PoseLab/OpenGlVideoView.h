#pragma once

#include <QOpenGLWidget>
#include <QAbstractVideoSurface>

#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>

class OverlayPainter;
class OpenGlVideoSurface;

class OpenGlVideoView : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT

public:
	OpenGlVideoView(QWidget *parent);
	~OpenGlVideoView();

	OpenGlVideoSurface* surface;

//virtual bool isValid() const {
//	return (wasInitialized());
//}
//
//bool wasInitialized() const {
//	return (vertexArrayObject.isCreated());
//}
	
	void setOverlay(OverlayPainter& overlay);

protected slots:
	void setFrame(QVideoFrame& frame);
	//void setFrame(QImage frame);
	void setSurfaceFormat(const QVideoSurfaceFormat& format);

protected:
	void initializeGL();
	void resizeGL(int w, int h)	override;
	void paintGL() override;

	QOpenGLVertexArrayObject vertexArrayObject;
	QOpenGLBuffer quadVertexBuffer, quadIndexBuffer;
	QOpenGLShaderProgram program;
	QOpenGLTexture* videoTexture;
	OverlayPainter* overlay;

	int localWidth, localHeight;
	int width, height;
	qreal devicePixelRatio;
	int videoWidth, videoHeight;
};
