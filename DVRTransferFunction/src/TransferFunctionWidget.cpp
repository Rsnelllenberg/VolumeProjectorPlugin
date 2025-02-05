#include "TransferFunctionWidget.h"

#include <CoreInterface.h>

#include <util/Exception.h>

#include <vector>

#include <QDebug>
#include <QGuiApplication>
#include <QMatrix4x4>
#include <QOpenGLFramebufferObject>
#include <QPainter>
#include <QSize>
#include <QWheelEvent>
#include <QWindow>
#include <QRectF>

#include <math.h>

#include "TransferFunctionPlugin.h"

using namespace mv;

namespace
{
    Bounds getDataBounds(const std::vector<Vector2f>& points)
    {
        Bounds bounds = Bounds::Max;

        for (const Vector2f& point : points)
        {
            bounds.setLeft(std::min(point.x, bounds.getLeft()));
            bounds.setRight(std::max(point.x, bounds.getRight()));
            bounds.setBottom(std::min(point.y, bounds.getBottom()));
            bounds.setTop(std::max(point.y, bounds.getTop()));
        }

        return bounds;
    }

    void translateBounds(Bounds& b, float x, float y)
    {
        b.setLeft(b.getLeft() + x);
        b.setRight(b.getRight() + x);
        b.setBottom(b.getBottom() + y);
        b.setTop(b.getTop() + y);
    }
}


TransferFunctionWidget::TransferFunctionWidget() :
    QOpenGLWidget(),
    _pointRenderer(),
    _isInitialized(false),
    _backgroundColor(255, 255, 255, 255),
    _widgetSizeInfo(),
    _dataRectangleAction(this, "Data rectangle"),
    _navigationAction(this, "Navigation"),
    _pixelSelectionTool(this),
    _pixelRatio(1.0),
    _mousePositions(),
    _mouseIsPressed(false),
	_materialMap(1024, 1024, QImage::Format_ARGB32),
	_areaSelectionBounds(0, 0, 0, 0) // Invalid Rectangle set to signal that no area is selected
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    setAcceptDrops(true);
    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);
    grabGesture(Qt::PinchGesture);
    //setAttribute(Qt::WA_TranslucentBackground);
    installEventFilter(this);

    _navigationAction.initialize(this);

    _pixelSelectionTool.setEnabled(true);
    _pixelSelectionTool.setMainColor(QColor(Qt::black));
    _pixelSelectionTool.setFixedBrushRadiusModifier(Qt::AltModifier);
    setSelectionOutlineHaloEnabled(false);

    connect(&_pixelSelectionTool, &PixelSelectionTool::shapeChanged, [this]() {
        if (isInitialized())
            update();
    });

    connect(&_pixelSelectionTool, &PixelSelectionTool::ended, [this]() {
        if (isInitialized() && !_selectedObject && _pixelSelectionTool.isEnabled() && _areaSelectionBounds.isValid() && _createShape) {
            _interactiveShapes.push_back(InteractiveShape(_pixelSelectionTool.getAreaPixmap().copy(_areaSelectionBounds), _areaSelectionBounds));
			_areaSelectionBounds = QRect(0, 0, 0, 0); // Invalid Rectangle set to signal that no area is selected
            update();
            _pixelSelectionTool.setMainColor(QColor(std::rand() % 256, std::rand() % 256, std::rand() % 256));
        }
        });

    QSurfaceFormat surfaceFormat;
    surfaceFormat.setRenderableType(QSurfaceFormat::OpenGL);
    surfaceFormat.setVersion(3, 3); 
    surfaceFormat.setProfile(QSurfaceFormat::CoreProfile);
    surfaceFormat.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    surfaceFormat.setSamples(16);
    surfaceFormat.setStencilBufferSize(8);

#ifdef _DEBUG
    surfaceFormat.setOption(QSurfaceFormat::DebugContext);
#endif

    setFormat(surfaceFormat);
    
    // Call updatePixelRatio when the window is moved between hi and low dpi screens
    // e.g., from a laptop display to a projector
    // Wait with the connection until we are sure that the window is created
    connect(this, &TransferFunctionWidget::created, this, [this](){
        [[maybe_unused]] auto windowID = this->window()->winId(); // This is needed to produce a valid windowHandle on some systems

        QWindow* winHandle = windowHandle();

        // On some systems we might need to use a different windowHandle
        if(!winHandle)
        {
            const QWidget* nativeParent = nativeParentWidget();
            winHandle = nativeParent->windowHandle();
        }
        
        if(winHandle == nullptr)
        {
            qDebug() << "TransferFunctionWidget: Not connecting updatePixelRatio - could not get window handle";
            return;
        }

        QObject::connect(winHandle, &QWindow::screenChanged, this, &TransferFunctionWidget::updatePixelRatio, Qt::UniqueConnection);
    });

    connect(&_navigationAction.getZoomRectangleAction(), &DecimalRectangleAction::rectangleChanged, this, [this]() -> void {
        auto& zoomRectangleAction = _navigationAction.getZoomRectangleAction();

        const auto zoomBounds = zoomRectangleAction.getBounds();

        _pointRenderer.setViewBounds(zoomBounds);
        _navigationAction.getZoomDataExtentsAction().setEnabled(zoomBounds != _dataRectangleAction.getBounds());

        update();
    });
}

bool TransferFunctionWidget::event(QEvent* event)
{
    if (!event)
        return QOpenGLWidget::event(event);
    switch (event->type())
    {
        // Set navigation flag on Alt press/release
        case QEvent::KeyRelease:
        {
            if (const auto* keyEvent = static_cast<QKeyEvent*>(event))
                if (keyEvent->key() == Qt::Key_Alt)
                    _mouseIsPressed = false;

        }
        case QEvent::KeyPress:
        {
            if (const auto* keyEvent = static_cast<QKeyEvent*>(event))
                if (keyEvent->key() == Qt::Key_Alt)
                    _mouseIsPressed = true;

        }
    }

    // Interactions when Alt is pressed
    //if (isInitialized() && QGuiApplication::keyboardModifiers() == Qt::AltModifier) {

    //    switch (event->type())
    //    {
    //        case QEvent::Wheel:
    //        {
    //            // Scroll to zoom
    //            if (auto* wheelEvent = static_cast<QWheelEvent*>(event))
    //                zoomAround(wheelEvent->position().toPoint(), wheelEvent->angleDelta().x() / 1200.f);

    //            break;
    //        }

    //        case QEvent::MouseButtonPress:
    //        {
    //            if (const auto* mouseEvent = static_cast<QMouseEvent*>(event))
    //            {
    //                if(mouseEvent->button() == Qt::MiddleButton)
    //                    resetView();

    //                // Navigation
    //                if (mouseEvent->buttons() == Qt::LeftButton)
    //                {
    //                    _pixelSelectionTool.setEnabled(false);
    //                    setCursor(Qt::ClosedHandCursor);
    //                    _mousePositions << mouseEvent->pos();
    //                    update();
    //                }
    //            }

    //            break;
    //        }

    //        case QEvent::MouseButtonRelease:
    //        {
    //            _pixelSelectionTool.setEnabled(true);
    //            setCursor(Qt::ArrowCursor);
    //            _mousePositions.clear();
    //            update();

    //            break;
    //        }

    //        case QEvent::MouseMove:
    //        {
    //            if (const auto* mouseEvent = static_cast<QMouseEvent*>(event))
    //            {
    //                _mousePositions << mouseEvent->pos();

    //                if (mouseEvent->buttons() == Qt::LeftButton && _mousePositions.size() >= 2) 
    //                {
    //                    const auto& previousMousePosition   = _mousePositions[_mousePositions.size() - 2];
    //                    const auto& currentMousePosition    = _mousePositions[_mousePositions.size() - 1];
    //                    const auto panVector                = currentMousePosition - previousMousePosition;

    //                    panBy(panVector);
    //                }
    //            }

    //            break;
    //        }

    //    }
    //
    //}
    // The below three cases are for interactive objects
    switch (event->type())
    {
        case QEvent::MouseButtonPress:
        {
            if (const auto* mouseEvent = static_cast<QMouseEvent*>(event)) {
                _mouseIsPressed = true;
                _mousePositions << mouseEvent->pos();
                for (auto it = _interactiveShapes.begin(); it != _interactiveShapes.end(); ++it) {
                    if (it->isNearTopRightCorner(mouseEvent->pos())) {
                        _interactiveShapes.erase(it);
						qDebug() << "Object deleted";
                        update();
                        break;
                    }
                    if (it->contains(mouseEvent->pos())) {
                        qDebug() << "Object selected";
                        _createShape = false;
                        _pixelSelectionTool.setEnabled(false);

                        it->setSelected(true);
                        _selectedObject = &(*it);
                        break;
                    }
                }
                if (!_selectedObject)
                    _createShape = true;
                break;
            }
        }
        case QEvent::MouseMove:
        {
            if (_mouseIsPressed) {
                if (const auto* mouseEvent = static_cast<QMouseEvent*>(event)) {
                    if (_selectedObject) {
                        auto delta = mouseEvent->pos() - _mousePositions[_mousePositions.size() - 1];
                        _selectedObject->moveBy(delta);
                    }
                    else {
                        if (_pixelSelectionTool.getType() == PixelSelectionType::Rectangle) {
                            QPoint topLeft(
                                std::min(_mousePositions[0].x(), mouseEvent->pos().x()),
                                std::min(_mousePositions[0].y(), mouseEvent->pos().y())
                            );

                            QPoint bottomRight(
                                std::max(_mousePositions[0].x(), mouseEvent->pos().x()),
                                std::max(_mousePositions[0].y(), mouseEvent->pos().y())
                            );

                            _areaSelectionBounds = QRect(topLeft, bottomRight); // Needed to create a valid QRect object
                        }
                        else
                            _areaSelectionBounds = getMousePositionsBounds(mouseEvent->pos());
                    }
                    _mousePositions << mouseEvent->pos();
                    update();
                }
            }
            break;
        }
        case QEvent::MouseButtonRelease:
        {
            if (_selectedObject) {
                _selectedObject->setSelected(false);
                _selectedObject = nullptr;

                _pixelSelectionTool.setEnabled(true);
            }
            _mousePositions.clear();
            _mouseIsPressed = false;
            update();
            break;
        }
    }

    return QOpenGLWidget::event(event);
}

QRect TransferFunctionWidget::getMousePositionsBounds(QPoint newMousePosition) {
    if (!_areaSelectionBounds.isValid()) {
		_areaSelectionBounds = QRect(_mousePositions[0], _mousePositions[0]);
    }
	int left = std::min(_areaSelectionBounds.left(), newMousePosition.x());
	int right = std::max(_areaSelectionBounds.right(), newMousePosition.x());
	int top = std::min(_areaSelectionBounds.top(), newMousePosition.y());
	int bottom = std::max(_areaSelectionBounds.bottom(), newMousePosition.y());
    return QRect(QPoint(left, top), QPoint(right, bottom));
}

void TransferFunctionWidget::resetView()
{
    _navigationAction.getZoomRectangleAction().setBounds(_dataRectangleAction.getBounds());
}

void TransferFunctionWidget::panBy(const QPointF& to)
{
    auto& zoomRectangleAction = _navigationAction.getZoomRectangleAction();

    const auto moveBy = QPointF(to.x() / _widgetSizeInfo.width * zoomRectangleAction.getWidth() * _widgetSizeInfo.ratioWidth * -1.f,
                                to.y() / _widgetSizeInfo.height * zoomRectangleAction.getHeight() * _widgetSizeInfo.ratioHeight);

    zoomRectangleAction.translateBy({ moveBy.x(), moveBy.y() });

    update();
}

void TransferFunctionWidget::zoomAround(const QPointF& at, float factor)
{
    auto& zoomRectangleAction = _navigationAction.getZoomRectangleAction();

    // the widget might have a different aspect ratio than the square opengl viewport
    const auto offsetBounds = QPointF(zoomRectangleAction.getWidth()  * (0.5f * (1 - _widgetSizeInfo.ratioWidth)),
                                      zoomRectangleAction.getHeight() * (0.5f * (1 - _widgetSizeInfo.ratioHeight)) * -1.f);

    const auto originBounds = QPointF(zoomRectangleAction.getLeft(), zoomRectangleAction.getTop());

    // translate mouse point in widget to mouse point in bounds coordinates
    const auto atTransformed = QPointF(at.x() / _widgetSizeInfo.width * zoomRectangleAction.getWidth() * _widgetSizeInfo.ratioWidth,
                                       at.y() / _widgetSizeInfo.height * zoomRectangleAction.getHeight() * _widgetSizeInfo.ratioHeight * -1.f);

    const auto atInBounds = originBounds + offsetBounds + atTransformed;

    // ensure mouse position is the same after zooming
    const auto currentBoundCenter = zoomRectangleAction.getCenter();

    float moveMouseX = (atInBounds.x() - currentBoundCenter.first) * factor;
    float moveMouseY = (atInBounds.y() - currentBoundCenter.second) * factor;

    // zoom and move view
    zoomRectangleAction.translateBy({ moveMouseX, moveMouseY });
    zoomRectangleAction.expandBy(-1.f * factor);

    update();
}

bool TransferFunctionWidget::isInitialized() const
{
    return _isInitialized;
}

PixelSelectionTool& TransferFunctionWidget::getPixelSelectionTool()
{
    return _pixelSelectionTool;
}

// Positions need to be passed as a pointer as we need to store them locally in order
// to be able to find the subset of data that's part of a selection. If passed
// by reference then we can upload the data to the GPU, but not store it in the widget.
void TransferFunctionWidget::setData(const std::vector<Vector2f>* points)
{
    auto dataBounds = getDataBounds(*points);

    // pass un-adjusted data bounds to renderer for 2D colormapping
    _pointRenderer.setDataBounds(dataBounds);

    const auto shouldSetBounds = (mv::projects().isOpeningProject() || mv::projects().isImportingProject()) ? false : !_navigationAction.getFreezeZoomAction().isChecked();

    if (shouldSetBounds)
        _pointRenderer.setViewBounds(dataBounds);


    _dataRectangleAction.setBounds(dataBounds);

    if (shouldSetBounds)
        _navigationAction.getZoomRectangleAction().setBounds(dataBounds);

    _pointRenderer.setData(*points);
    update();
}

QColor TransferFunctionWidget::getBackgroundColor() const
{
    return _backgroundColor;
}

void TransferFunctionWidget::setBackgroundColor(QColor color)
{
    _backgroundColor = color;

    update();
}

void TransferFunctionWidget::setHighlights(const std::vector<char>& highlights, const std::int32_t& numSelectedPoints)
{
    _pointRenderer.setHighlights(highlights, numSelectedPoints);

    update();
}

void TransferFunctionWidget::setPointSize(float pointSize)
{
    _pointRenderer.setPointSize(pointSize);
    update();
}

void TransferFunctionWidget::setPointOpacity(float pointOpacity)
{
    _pointRenderer.setAlpha(pointOpacity);
    update();
}


void TransferFunctionWidget::showHighlights(bool show)
{
    _pointRenderer.setSelectionOutlineScale(show ? 0.5f : 0);
    update();
}

PointSelectionDisplayMode TransferFunctionWidget::getSelectionDisplayMode() const
{
    return _pointRenderer.getSelectionDisplayMode();
}

void TransferFunctionWidget::setSelectionDisplayMode(PointSelectionDisplayMode selectionDisplayMode)
{
    _pointRenderer.setSelectionDisplayMode(selectionDisplayMode);

    update();
}

void TransferFunctionWidget::setSelectionOutlineHaloEnabled(bool selectionOutlineHaloEnabled)
{
    _pointRenderer.setSelectionHaloEnabled(selectionOutlineHaloEnabled);

    update();
}

void TransferFunctionWidget::setRandomizedDepthEnabled(bool randomizedDepth)
{
    _pointRenderer.setRandomizedDepthEnabled(randomizedDepth);

    update();
}

bool TransferFunctionWidget::getRandomizedDepthEnabled() const
{
    return _pointRenderer.getRandomizedDepthEnabled();
}

void TransferFunctionWidget::initializeGL()
{
    initializeOpenGLFunctions();

#ifdef SCATTER_PLOT_WIDGET_VERBOSE
    qDebug() << "Initializing transferFunction widget with context: " << context();

    std::string versionString = std::string((const char*) glGetString(GL_VERSION));

    qDebug() << versionString.c_str();
#endif

    connect(context(), &QOpenGLContext::aboutToBeDestroyed, this, &TransferFunctionWidget::cleanup);

    // Initialize renderers
    _pointRenderer.init();

    // Set a default color map for both renderers
    _pointRenderer.setScalarEffect(PointEffect::Color);

    _pointRenderer.setPointScaling(Absolute);
    _pointRenderer.setSelectionOutlineColor(Vector3f(1, 0, 0));

    // OpenGL is initialized
    _isInitialized = true;

    emit initialized();
}

void TransferFunctionWidget::resizeGL(int w, int h)
{
    _widgetSizeInfo.width       = static_cast<float>(w);
    _widgetSizeInfo.height      = static_cast<float>(h);
    _widgetSizeInfo.minWH       = _widgetSizeInfo.width < _widgetSizeInfo.height ? _widgetSizeInfo.width : _widgetSizeInfo.height;
    _widgetSizeInfo.ratioWidth  = _widgetSizeInfo.width / _widgetSizeInfo.minWH;
    _widgetSizeInfo.ratioHeight = _widgetSizeInfo.height / _widgetSizeInfo.minWH;

    // we need this here as we do not have the screen yet to get the actual devicePixelRatio when the view is created
    _pixelRatio = devicePixelRatio();

    // Pixel ratio tells us how many pixels map to a point
    // That is needed as macOS calculates in points and we do in pixels
    // On macOS high dpi displays pixel ration is 2
    w *= _pixelRatio;
    h *= _pixelRatio;

    _pointRenderer.resize(QSize(w, h));
}

void TransferFunctionWidget::paintGL()
{
    try {
        QPainter painter;

        // Begin mixed OpenGL/native painting
        if (!painter.begin(this))
            throw std::runtime_error("Unable to begin painting");

        // Draw layers with OpenGL
        painter.beginNativePainting();
        {
            // Bind the framebuffer belonging to the widget
            // glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

            // Clear the widget to the background color
            glClearColor(_backgroundColor.redF(), _backgroundColor.greenF(), _backgroundColor.blueF(), _backgroundColor.alphaF());
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Reset the blending function
            glEnable(GL_BLEND);

            if (getRandomizedDepthEnabled())
                glEnable(GL_DEPTH_TEST);

            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
               
            _pointRenderer.render();
        }
        painter.endNativePainting();

        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

        QImage pixelSelectionToolsImage(size(), QImage::Format_ARGB32);

        pixelSelectionToolsImage.fill(Qt::transparent);

        paintPixelSelectionToolNative(_pixelSelectionTool, pixelSelectionToolsImage);
        // Draw interactive objects
        for (const auto& obj : _interactiveShapes) {
            obj.draw(painter, true);
        }

        painter.drawImage(0, 0, pixelSelectionToolsImage);

        painter.end();
    }
    catch (std::exception& e)
    {
        exceptionMessageBox("Rendering failed", e);
    }
    catch (...) {
        exceptionMessageBox("Rendering failed");
    }
}

void TransferFunctionWidget::paintPixelSelectionToolNative(PixelSelectionTool& pixelSelectionTool, QImage& image) const
{
    if (!pixelSelectionTool.isEnabled())
        return;

    QPainter pixelSelectionToolImagePainter(&image);

    pixelSelectionToolImagePainter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    pixelSelectionToolImagePainter.drawPixmap(rect(), pixelSelectionTool.getShapePixmap());
    pixelSelectionToolImagePainter.drawPixmap(rect(), pixelSelectionTool.getAreaPixmap());
}

void TransferFunctionWidget::cleanup()
{
    qDebug() << "Deleting transferFunction widget, performing clean up...";
    _isInitialized = false;

    makeCurrent();
    _pointRenderer.destroy();
}

void TransferFunctionWidget::updatePixelRatio()
{
    float pixelRatio = devicePixelRatio();
    
#ifdef SCATTER_PLOT_WIDGET_VERBOSE
    qDebug() << "Window moved to screen " << window()->screen() << ".";
    qDebug() << "Pixelratio before was " << _pixelRatio << ". New pixelratio is: " << pixelRatio << ".";
#endif // SCATTER_PLOT_WIDGET_VERBOSE
    
    // we only update if the ratio actually changed
    if( _pixelRatio != pixelRatio )
    {
        _pixelRatio = pixelRatio;
        resizeGL(width(), height());
        update();
    }
}

TransferFunctionWidget::~TransferFunctionWidget()
{
    disconnect(QOpenGLWidget::context(), &QOpenGLContext::aboutToBeDestroyed, this, &TransferFunctionWidget::cleanup);
    cleanup();
}
