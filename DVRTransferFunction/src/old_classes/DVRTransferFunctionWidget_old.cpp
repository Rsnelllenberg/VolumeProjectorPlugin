#include "DVRTransferFunctionWidget.h"

#include <util/Exception.h>

#include <QEvent>
#include <QWindow>
#include <QMouseEvent>
#include <QWheelEvent>

TransferFunctionWidget::TransferFunctionWidget() :
    QOpenGLWidget(),
    _isInitialized(false),
    _volumeRenderer(),
    _pixelRatio(1.0f),
    _bounds(),
    _mousePressed(false),
    _isNavigating(false)
{
    setAcceptDrops(true);

    QSurfaceFormat surfaceFormat;
    surfaceFormat.setRenderableType(QSurfaceFormat::OpenGL);

    // Ask for an different OpenGL versions depending on OS
#if defined(__APPLE__) 
    surfaceFormat.setVersion(4, 1); // https://support.apple.com/en-us/101525
    surfaceFormat.setProfile(QSurfaceFormat::CoreProfile);
#elif defined(__linux__ )
    surfaceFormat.setVersion(4, 2); // glxinfo | grep "OpenGL version"
    surfaceFormat.setProfile(QSurfaceFormat::CompatibilityProfile);
#else
    surfaceFormat.setVersion(4, 3);
    surfaceFormat.setProfile(QSurfaceFormat::CoreProfile);
#endif

#ifdef _DEBUG
    surfaceFormat.setOption(QSurfaceFormat::DebugContext);
#endif

    surfaceFormat.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    surfaceFormat.setSamples(16);

    setFormat(surfaceFormat);
    this->installEventFilter(this);

    // Call updatePixelRatio when the window is moved between hi and low dpi screens
    // e.g., from a laptop display to a projector
    // Wait with the connection until we are sure that the window is created
    connect(this, &TransferFunctionWidget::created, this, [this]() {
        [[maybe_unused]] auto windowID = this->window()->winId(); // This is needed to produce a valid windowHandle on some systems

        QWindow* winHandle = windowHandle();

        // On some systems we might need to use a different windowHandle
        if (!winHandle)
        {
            const QWidget* nativeParent = nativeParentWidget();
            winHandle = nativeParent->windowHandle();
        }

        if (winHandle == nullptr)
        {
            qDebug() << "ScatterplotWidget: Not connecting updatePixelRatio - could not get window handle";
            return;
        }

        QObject::connect(winHandle, &QWindow::screenChanged, this, &DVRWidget::updatePixelRatio, Qt::UniqueConnection);
        });
}

TransferFunctionWidget::~TransferFunctionWidget()
{
    cleanup();
}

void TransferFunctionWidget::setData()
{


    // Calls paintGL()
    update();
}

void TransferFunctionWidget::initializeGL()
{
    qDebug() << "Initializing DVRWidget";
    initializeOpenGLFunctions();

    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &DVRWidget::cleanup);
    

    // Initialize renderers
    _volumeRenderer.init();

    mv::Vector3f center = _volumeRenderer.getVolumeSize();
    _camera.setDistance(center.x * 2);
    _camera.setCenter(QVector3D(center.x, center.y, center.z) / 2);

    _volumeRenderer.setCamera(_camera);
    _volumeRenderer.setClippingPlaneBoundery(_minClippingPlane, _maxClippingPlane);

    // OpenGL is initialized
    _isInitialized = true;

    emit initialized();
}

void TransferFunctionWidget::updatePixelRatio()
{
    float pixelRatio = devicePixelRatio();

    // we only update if the ratio actually changed
    if (_pixelRatio != pixelRatio)
    {
        _pixelRatio = pixelRatio;
        resizeGL(width(), height());
        update();
    }
}

void TransferFunctionWidget::resizeGL(int w, int h)
{
    // we need this here as we do not have the screen yet to get the actual devicePixelRatio when the view is created
    _pixelRatio = devicePixelRatio();
    
    // Pixel ration tells us how many pixels map to a point
    // That is needed as macOS calculates in points and we do in pixels
    // On macOS high dpi displays pixel ration is 2
    w *= _pixelRatio;
    h *= _pixelRatio;

    _camera.setViewport(w, h);
    _volumeRenderer.resize(QSize(w, h));
}

void TransferFunctionWidget::paintGL()
{
    _volumeRenderer.setCamera(_camera);
    _volumeRenderer.setDefaultFramebuffer(defaultFramebufferObject());
    _volumeRenderer.render();

}

bool TransferFunctionWidget::event(QEvent* event)
{
    if (isInitialized()) {

        switch (event->type())
        {
        case QEvent::Wheel:
        {
            // Scroll to zoom
            if (auto* wheelEvent = static_cast<QWheelEvent*>(event)) {
                qDebug() << "Mouse wheel" << wheelEvent->angleDelta().y() << " ";
                _camera.mouseWheel(wheelEvent->angleDelta().y()); // 120 is the typical delta for one wheel step
                update();
            }
            break;
        }

        case QEvent::MouseButtonPress:
        {

            if (auto* mouseEvent = static_cast<QMouseEvent*>(event))
            {
                _mousePressed = true;
                qDebug() << "mouse press";
                _camera.mousePress(mouseEvent->position());
                _isNavigating = true;
            }

            break;
        }

        case QEvent::MouseButtonRelease:
        {
            if (_isNavigating)
            {
                _isNavigating = false;
                _mousePressed = false;
                update();
            }

            break;
        }

        case QEvent::MouseMove:
        {
            if (auto* mouseEvent = static_cast<QMouseEvent*>(event))
            {
                if (_isNavigating)
                {
                    QPointF mousePos = mouseEvent->position();
                    if (mouseEvent->buttons() == Qt::RightButton) {
                        _camera.shiftCenter(mousePos);
                        update();
                    }

                    if (mouseEvent->buttons() == Qt::LeftButton) {
                        _camera.rotateCamera(mousePos);
                        update();
                    }
                }
            }

            break;
        }
        }
    }

    return QOpenGLWidget::event(event);
}

void TransferFunctionWidget::cleanup()
{
    qDebug() << "Deleting widget, performing clean up...";
    _isInitialized = false;

    makeCurrent();
    _volumeRenderer.destroy();
}
