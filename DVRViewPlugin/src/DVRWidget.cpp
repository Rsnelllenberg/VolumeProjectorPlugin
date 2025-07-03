#include "DVRWidget.h"

#include <util/Exception.h>

#include <QEvent>
#include <QWindow>
#include <QMouseEvent>
#include <QWheelEvent>

DVRWidget::DVRWidget() : 
    QOpenGLWidget(),
    _isInitialized(false),
    _volumeRenderer(),
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

    // Call updatePixelRatio when the window is moved between hi and low dpi screens
    // e.g., from a laptop display to a projector
    // Wait with the connection until we are sure that the window is created
    connect(this, &DVRWidget::created, this, [this]() {
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
            qDebug() << "TransferFunctionWidget: Not connecting updatePixelRatio - could not get window handle";
            return;
        }

        QObject::connect(winHandle, &QWindow::screenChanged, this, &DVRWidget::updatePixelRatio, Qt::UniqueConnection);
        });
}

DVRWidget::~DVRWidget()
{
    cleanup();
}

void DVRWidget::setData(const Dataset<Volumes>& dataset, std::vector<std::uint32_t>& dimensionIndices)
{
    _volumeDataset = dataset;

    //reset camera to fit new dataset
    mv::Vector3f center = _volumeDataset->getVolumeSize().toVector3f() / 2.0f;
    _camera.setDistance(_volumeDataset->getVolumeSize().width() * 2);
    _camera.setCenter(QVector3D(center.x, center.y, center.z));

    _volumeRenderer.setCompositeIndices(dimensionIndices);
    _volumeRenderer.setData(dataset);

    // Calls paintGL()
    update();
}

void DVRWidget::setTfTexture(const Dataset<Images>& tfTexture)
{
    _volumeRenderer.setTfTexture(tfTexture);
    update();
}

void DVRWidget::setReducedPosData(const Dataset<Points>& reducedPosData)
{
    _volumeRenderer.setReducedPosData(reducedPosData);
    update();
}

void DVRWidget::setMaterialTransitionTexture(const Dataset<Images>& materialTransitionTexture)
{
    _volumeRenderer.setMaterialTransitionTexture(materialTransitionTexture);
    update();
}

void DVRWidget::setMaterialPositionTexture(const Dataset<Images>& materialPositionTexture)
{
    _volumeRenderer.setMaterialPositionTexture(materialPositionTexture);
    update();
}

void DVRWidget::setClippingPlaneBoundery(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax)
{
    _minClippingPlane = mv::Vector3f(xMin, yMin, zMin);
    _maxClippingPlane = mv::Vector3f(xMax, yMax, zMax);

    _volumeRenderer.setClippingPlaneBoundery(_minClippingPlane, _maxClippingPlane);
}

void DVRWidget::setStepSize(float stepSize)
{
    _volumeRenderer.setStepSize(stepSize);
}

void DVRWidget::setRenderSpace(float xSize, float ySize, float zSize)
{
    _volumeRenderer.setRenderSpace(mv::Vector3f(xSize, ySize, zSize));
}

void DVRWidget::setUseCustomRenderSpace(bool useCustomRenderSpace)
{
    _volumeRenderer.setUseCustomRenderSpace(useCustomRenderSpace);
}

void DVRWidget::setRenderMode(const QString& renderMode)
{
    _volumeRenderer.setRenderMode(renderMode);
}

void DVRWidget::setMIPDimension(int mipDimension)
{
    _volumeRenderer.setMIPDimension(mipDimension);
}

void DVRWidget::setUseClutterRemover(bool useClutterRemover)
{
    _volumeRenderer.setUseClutterRemover(useClutterRemover);
}

void DVRWidget::setUseShading(bool useShading)
{
    _volumeRenderer.setUseShading(useShading);
}

void DVRWidget::setRenderCubeSize(float renderCubeSize)
{
    _volumeRenderer.setRenderCubeSize(renderCubeSize);
}

void DVRWidget::initializeGL()
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

void DVRWidget::updatePixelRatio()
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
    _volumeRenderer.resize(QSize(w, h));
}

void DVRWidget::paintGL()
{
    _volumeRenderer.setCamera(_camera);
    _volumeRenderer.setDefaultFramebuffer(defaultFramebufferObject());
    _volumeRenderer.render();
    if (_volumeRenderer.getFullRenderModeInProgress()) // We need to update the screen to add the next batch
    {
        update();
    }
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

void DVRWidget::cleanup()
{
    qDebug() << "Deleting widget, performing clean up...";
    _isInitialized = false;

    makeCurrent();
    _volumeRenderer.destroy();
}
