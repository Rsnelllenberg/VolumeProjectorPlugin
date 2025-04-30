#include "DVRWidget.h"
#include "TrackballCamera.h"

#include <QEvent>
#include <QMouseEvent>
#include <QWheelEvent>

DVRWidget::DVRWidget() :
    QOpenGLWidget()
    , _isInitialized(false)
    , _mousePressed(false)
    , _volumeRenderer()
    , _camera()
    , _backgroundColor(Qt::black)
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


    setFocusPolicy(Qt::FocusPolicy::ClickFocus);
    installEventFilter(this);
    _mousePressed = false;
    qDebug() << "Volume Widget created";
}

DVRWidget::~DVRWidget()
{
    cleanup(); 
}

void DVRWidget::setData(std::vector<float>& spatialData, std::vector<float>& valueData, int valueDims)
{
    makeCurrent();

    std::vector<mv::Vector3f> convertedSpatialData;
    for (size_t i = 0; i < spatialData.size(); i += 3) {
        convertedSpatialData.emplace_back(spatialData[i], spatialData[i + 1], spatialData[i + 2]);
    }

    std::vector<std::vector<float>> convertedValueData;
    for (size_t i = 0; i < valueData.size(); i += valueDims) {
        std::vector<float> valueSegment(valueData.begin() + i, valueData.begin() + i + valueDims);
        convertedValueData.push_back(valueSegment);
    }

    _volumeRenderer.setData(convertedSpatialData, convertedValueData);
    qDebug() << "Volume renderer data updated";
    update();
}

void DVRWidget::initializeGL()
{
    initializeOpenGLFunctions();

    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &DVRWidget::cleanup);
    qDebug() << "VolumeRendererWidget: InitializeGL";

    // Initialize renderers
    _volumeRenderer.init();
    qDebug() << "VolumeRendererWidget: InitializeGL Done";

    // OpenGL is initialized
    _isInitialized = true;
    emit initialized();

    _camera.setDistance(5.0f);
    mv::Vector3f center = _volumeRenderer.getVoxelBox().getCenter();
    _camera.setCenter(QVector3D(center.x, center.y, center.z));
}

void DVRWidget::resizeGL(int w, int h)
{
    _volumeRenderer.resize(w, h);
    _camera.setViewport(w, h);
}

void DVRWidget::paintGL()
{
    qDebug() << "PaintGL: DVRWidget";
    initializeOpenGLFunctions();
    // Bind the framebuffer belonging to the widget
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

    // Clear the widget to the background color
    glClearColor(_backgroundColor.redF(), _backgroundColor.greenF(), _backgroundColor.blueF(), _backgroundColor.alphaF());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // Reset the blending function
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    resizeGL(width(), height());
    _volumeRenderer.render(defaultFramebufferObject(), _camera);
}

void DVRWidget::cleanup()
{
    _isInitialized = false;
    _volumeRenderer.destroy();
    makeCurrent();
}

bool DVRWidget::eventFilter(QObject* target, QEvent* event)
{
    switch (event->type())
    {
    case QEvent::KeyRelease:
    {
        qDebug() << "Beep";
        makeCurrent();
        _volumeRenderer.reloadShader();
        break;
    }
    case QEvent::MouseButtonPress:
    {
        qDebug() << "Mouse press";
        auto mouseEvent = static_cast<QMouseEvent*>(event);

        QPointF mousePos = mouseEvent->position();
        bool isRightButton = (mouseEvent->button() == Qt::RightButton);
        _camera.mousePress(mousePos);

        _mousePressed = true;
        break;
    }
    case QEvent::MouseMove:
    {
        if (!_mousePressed)
            break;

        auto mouseEvent = static_cast<QMouseEvent*>(event);

        QPointF mousePos = mouseEvent->position();
        bool isRightButton = (mouseEvent->buttons() & Qt::RightButton);
        _camera.mouseMove(mousePos, isRightButton);

        update();

        break;
    }
    case QEvent::MouseButtonRelease:
    {
        _mousePressed = false;
        break;
    }
    case QEvent::Wheel:
    {
        auto wheelEvent = static_cast<QWheelEvent*>(event);
        qDebug() << "Mouse wheel" << wheelEvent->angleDelta().y() << " ";
        _camera.mouseWheel(wheelEvent->angleDelta().y()); // 120 is the typical delta for one wheel step
        update();
        break;
    }
    paintGL(); // update the view after the event
    return QObject::eventFilter(target, event);
    }
}
