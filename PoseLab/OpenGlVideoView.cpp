#include "pch.h"

#include <QDesktopwidget>
#include <QScreen>
#include <QPainter>

#include "OverlayPainter.h"
#include "OpenGlVideoSurface.h"
#include "OpenGlVideoView.h"

OpenGlVideoView::OpenGlVideoView(QWidget* parent)
	: QOpenGLWidget(parent)
	, surface(new OpenGlVideoSurface(this))
	, overlay(nullptr)
	, videoTexture(nullptr)
	, videoWidth(640), videoHeight(360)
{
	// TODO Would block if same thread 
	connect(surface, &OpenGlVideoSurface::surfaceFormatChanged, this, &OpenGlVideoView::setSurfaceFormat);
	connect(surface, &OpenGlVideoSurface::aboutToPresent, this, &OpenGlVideoView::setFrame, Qt::ConnectionType::BlockingQueuedConnection);

	setAutoFillBackground(false);
}

OpenGlVideoView::~OpenGlVideoView() {
	disconnect(surface, &OpenGlVideoSurface::surfaceFormatChanged, this, &OpenGlVideoView::setSurfaceFormat);
	disconnect(surface, &OpenGlVideoSurface::aboutToPresent, this, &OpenGlVideoView::setFrame);
	delete surface;
}


// OpenGL code based on https://hackaday.io/project/28720-real-time-random-pixel-shuffling/details

void OpenGlVideoView::setSurfaceFormat(const QVideoSurfaceFormat& format) {
	videoWidth = format.frameWidth();
	videoHeight = format.frameHeight();

	setMinimumSize(videoWidth, videoHeight);
}

void OpenGlVideoView::setOverlay(OverlayPainter& overlay) {
	this->overlay = &overlay;
}

void OpenGlVideoView::initializeGL() {
	// initialize our gl calls and set the clear color
	initializeOpenGLFunctions();
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// create the vertex array object for feeding vertices to our shader programs
	vertexArrayObject.create();

	// create vertex buffer to hold corners of quadralateral
	quadVertexBuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
	quadVertexBuffer.create();
	quadVertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
	if (quadVertexBuffer.bind()) {
		// allocate the vertex buffer for holding the four corners of a rectangle
		quadVertexBuffer.allocate(16 * sizeof(float));
		float* buffer = (float*)quadVertexBuffer.map(QOpenGLBuffer::WriteOnly);
		if (buffer) {
			buffer[0] = -1.0;
			buffer[1] = -1.0;
			buffer[2] = 0.0;
			buffer[3] = 1.0;
			buffer[4] = +1.0;
			buffer[5] = -1.0;
			buffer[6] = 0.0;
			buffer[7] = 1.0;
			buffer[8] = +1.0;
			buffer[9] = +1.0;
			buffer[10] = 0.0;
			buffer[11] = 1.0;
			buffer[12] = -1.0;
			buffer[13] = +1.0;
			buffer[14] = 0.0;
			buffer[15] = 1.0;
			quadVertexBuffer.unmap();
		}
		else {
			qDebug() << QString("quadVertexBuffer not allocated.") << glGetError();
		}
		quadVertexBuffer.release();
	}

	// create index buffer to orderings of vertices forming polygon
	quadIndexBuffer = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
	quadIndexBuffer.create();
	quadIndexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
	if (quadIndexBuffer.bind()) {
		quadIndexBuffer.allocate(6 * sizeof(unsigned int));
		unsigned int* indices = (unsigned int*)quadIndexBuffer.map(QOpenGLBuffer::WriteOnly);
		if (indices) {
			indices[0] = 0;
			indices[1] = 1;
			indices[2] = 2;
			indices[3] = 0;
			indices[4] = 2;
			indices[5] = 3;
			quadIndexBuffer.unmap();
		}
		else {
			qDebug() << QString("indiceBufferA buffer mapped from GPU.");
		}
		quadIndexBuffer.release();
	}

	// create shader for showing the video not available image
	setlocale(LC_NUMERIC, "C");
	if (!program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Resources/Shaders/identity_vertex_shader.glsl")) {
		qDebug() << QString("Failed to load vertex shader.");
	}
	if (!program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/Resources/Shaders/identity_fragment_shader.glsl")) {
		qDebug() << QString("Failed to load fragment shader.");
	}
	program.link();
	setlocale(LC_ALL, "");
}

void OpenGlVideoView::setFrame(QVideoFrame& frame) {
	makeCurrent();

	// see if we need a new texture to hold the incoming video frame
	if (!videoTexture ||
		videoTexture->width() != frame.width() ||
		videoTexture->height() != frame.height()) {

		if (videoTexture) {
			delete videoTexture;
		}

		// create the gpu side texture buffer to hold the incoming video
		videoTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
		videoTexture->setSize(frame.width(), frame.height());
		videoTexture->setFormat(QOpenGLTexture::RGBA32F);
		videoTexture->setWrapMode(QOpenGLTexture::ClampToBorder);
		videoTexture->setMinificationFilter(QOpenGLTexture::Nearest);
		videoTexture->setMagnificationFilter(QOpenGLTexture::Nearest);
		videoTexture->allocateStorage();
	}

	QVideoFrame::PixelFormat format = frame.pixelFormat();
	if (format == QVideoFrame::Format_RGB32) {
		unsigned int bytesPerSample = frame.bytesPerLine() / frame.width() / 4;
		if (bytesPerSample == sizeof(unsigned char)) {
			videoTexture->setData(QOpenGLTexture::BGRA, QOpenGLTexture::UInt8,
				(const void*) frame.bits());
		}
	} else if (format == QVideoFrame::Format_BGR24) {
		unsigned int bytesPerSample = frame.bytesPerLine() / frame.width() / 3;
		if (bytesPerSample == sizeof(unsigned char)) {
			videoTexture->setData(QOpenGLTexture::BGR, QOpenGLTexture::UInt8,
				(const void*) frame.bits());
		}
	}

	update();
}

void OpenGlVideoView::resizeGL(int w, int h) {
	// Get the Desktop Widget so that we can get information about multiple monitors connected to the system.
	QDesktopWidget* dkWidget = QApplication::desktop();
	QList<QScreen*> screenList = QGuiApplication::screens();
	devicePixelRatio = screenList[dkWidget->screenNumber(this)]->devicePixelRatio();
	height = h;
	width = w;
	localHeight = h * devicePixelRatio;
	localWidth = w * devicePixelRatio;
	QOpenGLWidget::resizeGL(w, h);
}

void OpenGlVideoView::paintGL() {
	// set the video aspect

	const float aspectVideo = float(videoWidth) / float(videoHeight);
	const float aspectGl = float(localWidth) / float(localHeight);

	if (aspectGl < aspectVideo) {
		const float y = (localHeight - localHeight * aspectGl / aspectVideo) / 2;
		glViewport(0, y, localWidth, localHeight * aspectGl / aspectVideo);
	} else {
		const float x = (localWidth - localWidth * aspectVideo / aspectGl) / 2;
		glViewport(x, 0, localWidth * aspectVideo / aspectGl, localHeight);
	}

	// make sure we have a texture to show
	if (videoTexture) {
		if (program.bind()) {
			if (quadVertexBuffer.bind()) {
				if (quadIndexBuffer.bind()) {
					// set the active texture on the gpu
					glActiveTexture(GL_TEXTURE0);
					videoTexture->bind();
					program.setUniformValue("qt_texture", 0);

					// tell opengl programmable pipeline how to locate vertex position data
					program.setAttributeBuffer("qt_vertex", GL_FLOAT, 0, 4, 4 * sizeof(float));
					program.enableAttributeArray("qt_vertex");

					glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

					quadIndexBuffer.release();
				}
				quadVertexBuffer.release();
			}
			program.release();
		}
	}

	if (overlay) {
		overlay->begin(this);
		overlay->paint();
		overlay->end();
	}
}