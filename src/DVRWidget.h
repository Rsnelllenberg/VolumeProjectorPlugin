#pragma once

#include <renderers/PointRenderer.h>
#include "VolumeRenderer.h"
#include "TrackballCamera.h"
#include <graphics/Vector2f.h>
#include <graphics/Vector3f.h>
#include <graphics/Bounds.h>

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

#include <QColor>


using namespace mv;
using namespace mv::gui;

class DVRViewPlugin;

class DVRWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    DVRWidget();
    ~DVRWidget();
    
    /** Returns true when the widget was initialized and is ready to be used. */
    bool isInitialized() const { return _isInitialized;};

    /** Sets 2D point positions and visual properties in the renderer */
    void setData(const std::vector<mv::Vector2f>& points, float pointSize, float pointOpacity);

protected:
    // We have to override some QOpenGLWidget functions that handle the actual drawing
    void initializeGL()         override;
    void resizeGL(int w, int h) override;
    void paintGL()              override;
    void cleanup();

    bool eventFilter(QObject* target, QEvent* event);

signals:
    void initialized();

private:
    VolumeRenderer           _pointRenderer;     /* ManiVault OpenGL point renderer implementation */
    TrackballCamera          _camera;
    bool                     _mousePressed;
    QPointF                  _previousMousePos;

    float                   _pixelRatio;        /* Pixel ratio */
    QColor                  _backgroundColor;   /* Background color */
    bool                    _isInitialized;     /* Whether OpenGL is initialized */
};
