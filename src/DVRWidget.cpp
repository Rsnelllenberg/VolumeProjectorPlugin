#include "DVRWidget.h"
#include "TrackballCamera.h"

#include <QEvent>
#include <QMouseEvent>
#include <QWheelEvent>

DVRWidget::DVRWidget()
{
    setAcceptDrops(true);
    setFocusPolicy(Qt::FocusPolicy::ClickFocus);
    installEventFilter(this);
    _mousePressed = false;
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

    _camera.setDistance(5.0f);
}

void DVRWidget::resizeGL(int w, int h)
{
    _volumeRenderer.resize(w, h);
    _camera.setViewport(w, h);
}

void DVRWidget::paintGL()
{
    resizeGL(width(), height());
    _volumeRenderer.render(defaultFramebufferObject(), _camera);
}

void DVRWidget::cleanup()
{
    _isInitialized = false;

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
        _camera.mouseMove(mousePos);

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
        _camera.mouseWheel(wheelEvent->angleDelta().y() / 120.0f); // 120 is the typical delta for one wheel step
        update();
        break;
    }
    paintGL(); // update the view after the event
    return QObject::eventFilter(target, event);
    }
}
