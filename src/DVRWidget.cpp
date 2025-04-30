#include "DVRWidget.h"

#include <QEvent>
#include <QMouseEvent>

DVRWidget::DVRWidget()
{
    setAcceptDrops(true);
    setFocusPolicy(Qt::FocusPolicy::ClickFocus);
    installEventFilter(this);
}

void DVRWidget::setTexels(int width, int height, int depth, std::vector<float>& texels)
{
    makeCurrent();
    _volumeRenderer.setTexels(width, height, depth, texels);
}

void DVRWidget::setData(std::vector<float>& data)
{
    makeCurrent();
    _volumeRenderer.setData(data);
    qDebug() << "Volume renderer data updated";
    //update();
}

void DVRWidget::setColors(std::vector<float>& colors)
{
    makeCurrent();
    _volumeRenderer.setColors(colors);
}

void DVRWidget::setColormap(const QImage& colormap)
{
    _volumeRenderer.setColormap(colormap);
}

void DVRWidget::setCursorPoint(mv::Vector3f cursorPoint)
{
    _volumeRenderer.setCursorPoint(cursorPoint);
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

    _camPos.set(0, 0, _camDist);
}

void DVRWidget::resizeGL(int w, int h)
{
    _volumeRenderer.resize(w, h);
    //_windowSize.setWidth(w);
    //_windowSize.setHeight(h);

    //_pointRenderer.resize(QSize(w, h));
    //_densityRenderer.resize(QSize(w, h));

    //// Set matrix for normalizing from pixel coordinates to [0, 1]
    //toNormalisedCoordinates = Matrix3f(1.0f / w, 0, 0, 1.0f / h, 0, 0);

    //// Take the smallest dimensions in order to calculate the aspect ratio
    //int size = w < h ? w : h;

    //float wAspect = (float)w / size;
    //float hAspect = (float)h / size;
    //float wDiff = ((wAspect - 1) / 2.0);
    //float hDiff = ((hAspect - 1) / 2.0);

    //toIsotropicCoordinates = Matrix3f(wAspect, 0, 0, hAspect, -wDiff, -hDiff);
}

void DVRWidget::paintGL()
{
    int w = width();
    int h = height();

    float aspect = (float)w / h;

    _volumeRenderer.render(defaultFramebufferObject(), _camPos, _camAngle, aspect);
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

        QPointF mousePos = QPointF(mouseEvent->position().x(), mouseEvent->position().y());
        _previousMousePos = mousePos;

        _mousePressed = true;
        break;
    }
    case QEvent::MouseMove:
    {
        if (!_mousePressed)
            break;

        auto mouseEvent = static_cast<QMouseEvent*>(event);

        QPointF mousePos = QPointF(mouseEvent->position().x(), mouseEvent->position().y());

        QPointF diff = mousePos - _previousMousePos;

        _camAngle.y += diff.x() * 0.01f;
        _camAngle.x -= diff.y() * 0.01f;
        if (_camAngle.x > 3.14150) _camAngle.x = 3.14150;
        if (_camAngle.x < 0.0001) _camAngle.x = 0.0001;

        _camPos.x = _camDist * sin(_camAngle.x) * cos(_camAngle.y);
        _camPos.y = _camDist * cos(_camAngle.x);
        _camPos.z = _camDist * sin(_camAngle.x) * sin(_camAngle.y);

        update();

        _previousMousePos = mousePos;

        break;
    }
    }
    return QObject::eventFilter(target, event);
}
