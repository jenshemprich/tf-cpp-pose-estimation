#pragma once

#include <QOpenGLWidget>
#include <QAbstractVideoSurface>

#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>

class OpenGlVideoView : public QOpenGLWidget, protected QOpenGLFunctions
{
	Q_OBJECT

public:
	OpenGlVideoView(QWidget *parent);
	~OpenGlVideoView();

	class VideoSurface : public QAbstractVideoSurface {
	public:
		VideoSurface(OpenGlVideoView* display);
		// Inherited via QAbstractVideoSurface
		virtual QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType type = QAbstractVideoBuffer::NoHandle) const override;
		virtual bool present(const QVideoFrame& frame) override;
	private: 
		OpenGlVideoView* display;
	}  * surface;

	virtual bool isValid() const {
		return (wasInitialized());
	}

	bool wasInitialized() const {
		return (vertexArrayObject.isCreated());
	}

	void setFrame(const QVideoFrame& frame);
	//void setFrame(QImage frame);


	bool hasHeightForWidth() const override;
	int heightForWidth(int width) const override;

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

	int videoWidth, videoHeight;
};
