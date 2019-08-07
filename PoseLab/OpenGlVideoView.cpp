#include "pch.h"

#include <QDesktopwidget>
#include <QScreen>

#include "OpenGlVideoView.h"

OpenGlVideoView::OpenGlVideoView(QWidget *parent)
	: QOpenGLWidget(parent) 
	, surface(new VideoSurface(this))
	, videoTexture(nullptr)
	, videoWidth(640), videoHeight(360)
{}

OpenGlVideoView::~OpenGlVideoView()
{
	delete surface;
}

OpenGlVideoView::VideoSurface::VideoSurface(OpenGlVideoView* display) 
	: display(display)
{}

QList<QVideoFrame::PixelFormat> OpenGlVideoView::VideoSurface::supportedPixelFormats(QAbstractVideoBuffer::HandleType type) const {
	if (type == QAbstractVideoBuffer::NoHandle) {
		// TODO Camera provides RGBA but RGB is more than sufficient
		return QList<QVideoFrame::PixelFormat>() << QVideoFrame::Format_RGB32;
	}
	else {
		return QList<QVideoFrame::PixelFormat>();
	}
}

bool OpenGlVideoView::VideoSurface::present(const QVideoFrame& frame) {
	display->setFrame(frame);
	return true;
}


// OpenGL code based on https://hackaday.io/project/28720-real-time-random-pixel-shuffling/details

void OpenGlVideoView::initializeGL() {
	// INITIALIZE OUR GL CALLS AND SET THE CLEAR COLOR
	initializeOpenGLFunctions();
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// CREATE THE VERTEX ARRAY OBJECT FOR FEEDING VERTICES TO OUR SHADER PROGRAMS
	vertexArrayObject.create();
	vertexArrayObject.bind();

	// CREATE VERTEX BUFFER TO HOLD CORNERS OF QUADRALATERAL
	quadVertexBuffer = QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
	quadVertexBuffer.create();
	quadVertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
	if (quadVertexBuffer.bind()) {
		// ALLOCATE THE VERTEX BUFFER FOR HOLDING THE FOUR CORNERS OF A RECTANGLE
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

	// CREATE INDEX BUFFER TO ORDERINGS OF VERTICES FORMING POLYGON
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

	// CREATE SHADER FOR SHOWING THE VIDEO NOT AVAILABLE IMAGE
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

void OpenGlVideoView::setFrame(const QVideoFrame& frame) {
	QVideoFrame localFrame = frame;
	if (localFrame.map(QAbstractVideoBuffer::ReadOnly)) {
		emit frameArrived(localFrame);

		makeCurrent();

		// SEE IF WE NEED A NEW TEXTURE TO HOLD THE INCOMING VIDEO FRAME
		if (!videoTexture ||
			videoTexture->width() != localFrame.width() ||
			videoTexture->height() != localFrame.height()) {

			if (videoTexture) {
				delete videoTexture;
			}

			// CREATE THE GPU SIDE TEXTURE BUFFER TO HOLD THE INCOMING VIDEO
			videoTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
			videoTexture->setSize(localFrame.width(), localFrame.height());
			videoTexture->setFormat(QOpenGLTexture::RGBA32F);
			videoTexture->setWrapMode(QOpenGLTexture::ClampToBorder);
			videoTexture->setMinificationFilter(QOpenGLTexture::Nearest);
			videoTexture->setMagnificationFilter(QOpenGLTexture::Nearest);
			videoTexture->allocateStorage();
		}

		// UPLOAD THE CPU BUFFER TO THE GPU TEXTURE
		// COPY FRAME BUFFER TEXTURE FROM GPU TO LOCAL CPU BUFFER
		QVideoFrame::PixelFormat format = localFrame.pixelFormat();
		// if (format == QVideoFrame::Format_ARGB32) {
		if (format == QVideoFrame::Format_RGB32) {
				unsigned int bytesPerSample = localFrame.bytesPerLine() / localFrame.width() / 4;
			if (bytesPerSample == sizeof(unsigned char)) {
				videoTexture->setData(QOpenGLTexture::BGRA, QOpenGLTexture::UInt8,
					(const void*)localFrame.bits());
			}
		}
		localFrame.unmap();

		videoWidth = frame.width();
		videoHeight = frame.height();
		setMinimumSize(videoWidth, videoHeight);

		update();
	}
}

void OpenGlVideoView::resizeGL(int w, int h) {
	// Get the Desktop Widget so that we can get information about multiple monitors connected to the system.
	QDesktopWidget* dkWidget = QApplication::desktop();
	QList<QScreen*> screenList = QGuiApplication::screens();
	qreal devicePixelRatio = screenList[dkWidget->screenNumber(this)]->devicePixelRatio();
	localHeight = h * devicePixelRatio;
	localWidth = w * devicePixelRatio;
	QOpenGLWidget::resizeGL(w, h);
}

void OpenGlVideoView::paintGL() {
	// SET THE VIEW PORT
	glViewport(0, 0, localWidth, localHeight);
	//  skip glClear since the screen buffer is completely replaced by video frame, and depth is unused

	// MAKE SURE WE HAVE A TEXTURE TO SHOW
	if (videoTexture) {
		if (program.bind()) {
			if (quadVertexBuffer.bind()) {
				if (quadIndexBuffer.bind()) {
					// SET THE ACTIVE TEXTURE ON THE GPU
					glActiveTexture(GL_TEXTURE0);
					videoTexture->bind();
					program.setUniformValue("qt_texture", 0);

					// TELL OPENGL PROGRAMMABLE PIPELINE HOW TO LOCATE VERTEX POSITION DATA
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
}

bool OpenGlVideoView::hasHeightForWidth() const {
	return true;
}

int OpenGlVideoView::heightForWidth(int width) const {
	return width * videoHeight / videoWidth;
}
