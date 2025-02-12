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
    _pixelSelectionTool(this),
    _pixelRatio(1.0),
    _mousePositions(),
    _mouseIsPressed(false),
	_areaSelectionBounds(0, 0, 0, 0) // Invalid Rectangle set to signal that no area is selected
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    setAcceptDrops(true);
    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);
    grabGesture(Qt::PinchGesture);
    //setAttribute(Qt::WA_TranslucentBackground);
    installEventFilter(this);

    _pixelSelectionTool.setEnabled(true);
    _pixelSelectionTool.setMainColor(QColor(std::rand() % 256, std::rand() % 256, std::rand() % 256));
    _pixelSelectionTool.setFixedBrushRadiusModifier(Qt::AltModifier);
    setSelectionOutlineHaloEnabled(false);

    connect(&_pixelSelectionTool, &PixelSelectionTool::shapeChanged, [this]() {
        if (isInitialized())
            update();
    });

    connect(&_pixelSelectionTool, &PixelSelectionTool::ended, [this]() {
        if (isInitialized() && !_selectedObject && _pixelSelectionTool.isEnabled() && _areaSelectionBounds.isValid() && _createShape) {
            QRectF relativeRect(
                float(_areaSelectionBounds.left() - _boundsPointsWindow.left()) / _boundsPointsWindow.width(),
                float(_areaSelectionBounds.top() - _boundsPointsWindow.top()) / _boundsPointsWindow.height(),
                float(_areaSelectionBounds.width()) / _boundsPointsWindow.width(),
                float(_areaSelectionBounds.height()) / _boundsPointsWindow.height()
            );
            int borderWidth = 2;
            QRectF adjustedBounds = _areaSelectionBounds.adjusted(borderWidth, borderWidth, -borderWidth, -borderWidth); // The areapixmap doesn't contain the borders
            _interactiveShapes.push_back(InteractiveShape(_pixelSelectionTool.getAreaPixmap().copy(adjustedBounds.toRect()), relativeRect, _boundsPointsWindow));

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
}

bool TransferFunctionWidget::event(QEvent* event)
{
    if (!event)
        return QOpenGLWidget::event(event);

    // The below three cases are for interactive objects
    switch (event->type())
    {
    case QEvent::MouseButtonPress:
    {
        if (const auto* mouseEvent = static_cast<QMouseEvent*>(event)) {
            _mouseIsPressed = true;
            _mousePositions << mouseEvent->pos();
            for (auto shape = _interactiveShapes.begin(); shape != _interactiveShapes.end(); ++shape) {
                SelectedSide side;
                if (shape->isNearTopRightCorner(mouseEvent->pos())) {
                    _interactiveShapes.erase(shape);
                    qDebug() << "Object deleted";
                    update();
                    break;
                }
                if (shape->isNearSide(mouseEvent->pos(), side)) {
                    qDebug() << "Object side selected for resizing";
                    _createShape = false;
                    _pixelSelectionTool.setEnabled(false);

                    shape->setSelected(true);
                    _selectedObject = &(*shape);
					_selectedSide = side;
                    break;
                }
                if (shape->contains(mouseEvent->pos())) {
                    qDebug() << "Object selected";
                    _createShape = false;
                    _pixelSelectionTool.setEnabled(false);

                    shape->setSelected(true);
                    _selectedObject = &(*shape);
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
                    if (_selectedSide != SelectedSide::None) {
                        _selectedObject->resizeBy(delta, _selectedSide);
                    }
                    else {
                        _selectedObject->moveBy(delta);
                    }
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
            _selectedSide = SelectedSide::None;

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
    _pointRenderer.setBounds(dataBounds);

    _dataRectangleAction.setBounds(dataBounds);
	int w = width();
	int h = height();
    int size = w < h ? w : h;
    _boundsPointsWindow = QRect((w - size) / 2.0f, (h - size) / 2.0f, size, size);

	qDebug() << "Bounds of the points in the window: " << _boundsPointsWindow;

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

    _sourceDataset = mv::data().createDataset<Points>("Points", "Image data");
    _tfTextures = mv::data().createDataset<Images>("Images", "TransferFunction texture", _sourceDataset);


    _tfTextures->setType(ImageData::Type::Stack);
    _tfTextures->setNumberOfImages(1);
    _tfTextures->setNumberOfComponentsPerPixel(4);

	_boundsPointsWindow = QRect(0, 0, width(), height());

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

	_boundsPointsWindow = QRect((w - _widgetSizeInfo.minWH) / 2.0f, (h - _widgetSizeInfo.minWH) / 2.0f, _widgetSizeInfo.minWH, _widgetSizeInfo.minWH);
	qDebug() << "Bounds of the points in the window: " << _boundsPointsWindow;

    // Update the bounds for all interactive shapes
    for (auto& shape : _interactiveShapes) {
        shape.setBounds(_boundsPointsWindow);
    }


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

        updateTfTexture();
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

void TransferFunctionWidget::updateTfTexture()
{
	if (!_tfTextures.isValid())
		return;
    QImage materialMap = QImage(_boundsPointsWindow.width(), _boundsPointsWindow.height(), QImage::Format_ARGB32);
    QPainter painter(&materialMap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    for (const auto& obj : _interactiveShapes) {
        obj.draw(painter, false, false);
    }

    std::vector<float> data;
    data.reserve(materialMap.width() * materialMap.height() * 4);

    for (int y = materialMap.height() - 1; y >= 0; y--) {
        for (int x = 0; x < materialMap.width(); x++) {
            QColor color = materialMap.pixelColor(x, y);
            data.push_back(color.redF());
            data.push_back(color.greenF());
            data.push_back(color.blueF());
            data.push_back(color.alphaF());
        }
    }

	_sourceDataset->setData(data, 4); // update the data in the dataset

    _tfTextures->setImageSize(QSize(materialMap.width(), materialMap.height()));
    
    events().notifyDatasetDataChanged(_sourceDataset);
    events().notifyDatasetDataChanged(_tfTextures);
	
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
