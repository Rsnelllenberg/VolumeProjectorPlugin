#include "DVRWidget.h"

#include <util/Exception.h>

#include <QEvent>
#include <QWindow>
#include <QMouseEvent>
#include <QWheelEvent>

DVRWidget::DVRWidget() : 
    QOpenGLWidget(),
    _isInitialized(false),
    _backgroundColor(235, 235, 235, 255),
    _pointRenderer(),
    _pixelRatio(1.0f),
    _camera(),
    _mousePressed(false),
    _previousMousePos(0, 0),
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
}

DVRWidget::~DVRWidget()
{
    cleanup();
}

void DVRWidget::setData(const std::vector<mv::Vector2f>& points, float pointSize, float pointOpacity)
{
    const auto numPoints = points.size();

    constexpr mv::Vector3f pointColor = {0.f, 0.f, 0.f};

    // Calls paintGL()
    update();
}


void DVRWidget::initializeGL()
{
    initializeOpenGLFunctions();

    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &DVRWidget::cleanup);

    // Initialize renderers
    _pointRenderer.init();

    _camera.setDistance(5.0f);
    mv::Vector3f center = _pointRenderer.getVoxelBox().getCenter();
    _camera.setCenter(QVector3D(center.x, center.y, center.z));

    _pointRenderer.setCamera(TrackballCamera());

    // OpenGL is initialized
    _isInitialized = true;

    emit initialized();
}

void DVRWidget::resizeGL(int w, int h)
{
    // we need this here as we do not have the screen yet to get the actual devicePixelRatio when the view is created
    _pixelRatio = devicePixelRatio();
    
    // Pixel ration tells us how many pixels map to a point
    // That is needed as macOS calculates in points and we do in pixels
    // On macOS high dpi displays pixel ration is 2
    w *= _pixelRatio;
    h *= _pixelRatio;

    _camera.setViewport(w, h);
    _pointRenderer.resize(QSize(w, h));
}

void DVRWidget::paintGL()
{
    initializeOpenGLFunctions();
    // Bind the framebuffer belonging to the widget
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

    // Clear the widget to the background color
    glClearColor(_backgroundColor.redF(), _backgroundColor.greenF(), _backgroundColor.blueF(), _backgroundColor.alphaF());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Reset the blending function
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    _pointRenderer.setCamera(_camera);
    _pointRenderer.render();                
}

bool DVRWidget::event(QEvent* event)
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
                    _mousePressed = true;
                    QPointF mousePos = mouseEvent->position();
                    if (mouseEvent->button() == Qt::RightButton) {
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

        case QEvent::KeyRelease:
        {
            if (auto* keyEvent = static_cast<QKeyEvent*>(event))
            {
                // Reset navigation
                if (keyEvent && keyEvent->key() == Qt::Key_Alt)
                {
                    qDebug() << "Reset navigation";
                    _isNavigating = false;
                }

            }
            break;
        }
        }
    }

    return QOpenGLWidget::event(event);
}

void DVRWidget::cleanup()
{
    qDebug() << "Deleting widget, performing clean up...";
    _isInitialized = false;

    makeCurrent();
    _pointRenderer.destroy();
}
