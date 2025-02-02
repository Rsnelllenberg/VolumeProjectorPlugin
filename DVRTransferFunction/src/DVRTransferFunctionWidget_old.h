#pragma once

#include <renderers/PointRenderer.h>
#include <graphics/Vector2f.h>
#include <graphics/Vector3f.h>
#include <graphics/Bounds.h>

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

#include <QColor>


using namespace mv;
using namespace mv::gui;

class DVRTransferFunction;

class TransferFunctionWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    TransferFunctionWidget();
    ~TransferFunctionWidget();

    void updatePixelRatio();
    /** Returns true when the widget was initialized and is ready to be used. */
    bool isInitialized() const { return _isInitialized; };

    /** Sets 2D point positions and visual properties in the renderer */
    void setData(const std::vector<mv::Vector2f>& points, float pointSize, float pointOpacity);

protected:
    // We have to override some QOpenGLWidget functions that handle the actual drawing
    void initializeGL()         override;
    void resizeGL(int w, int h) override;
    void paintGL()              override;
    void cleanup();

signals:
    void initialized();

private:
    TransferFunctionRenderer    _TFRenderer;
    float                       _pixelRatio;                /* device pixel ratio */
    std::vector<Vector2f>       _points;                    /* 2D coordinates of points */
    std::vector<Vector3f>       _colors;                    /* Color of points - here we use a constant color for simplicity */
    Bounds                      _bounds;                    /* Min and max point coordinates for camera placement */
    QColor                      _backgroundColor;           /* Background color */
    bool                        _isInitialized;             /* Whether OpenGL is initialized */
    QVector<QPoint>             _mousePositions;            /* Recorded mouse positions */
	QVector<QPoint>             _previousMousePos;          /* Recorded previous mouse positions */
    bool                        _isNavigating;              /* Boolean determining whether view navigation is currently taking place or not */
	bool                        _mousePressed;              /* Boolean determining whether the mouse is currently pressed or not */
};
