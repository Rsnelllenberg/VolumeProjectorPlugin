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
#include <VolumeData/Volumes.h>
#include <ImageData/Images.h>


using namespace mv;
using namespace mv::gui;

class DVRViewPlugin;

class DVRWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    DVRWidget();
    ~DVRWidget();

    void updatePixelRatio();
    
    /** Returns true when the widget was initialized and is ready to be used. */
    bool isInitialized() const { return _isInitialized;};

    /** Sets 2D point positions and visual properties in the renderer */
    void setData(const Dataset<Volumes>& dataset, std::vector<std::uint32_t>& dimensionIndices);
    void setTfTexture(const Dataset<Images>& tfTexture);
    void setReducedPosData(const Dataset<Points>& reducedPosData);
    void setClippingPlaneBoundery(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax);
    void setRenderMode(const QString& renderMode);
    void setMIPDimension(int mipDimension);


protected:
    // We have to override some QOpenGLWidget functions that handle the actual drawing
    void initializeGL()         override;
    void resizeGL(int w, int h) override;
    void paintGL()              override;
    void cleanup();


    void showEvent(QShowEvent* event) Q_DECL_OVERRIDE
    {
        emit created();
        QWidget::showEvent(event);
    }

    bool event(QEvent* event) override;

signals:
    void initialized();
    void created();

private:
    VolumeRenderer           _volumeRenderer;     /* ManiVault OpenGL point renderer implementation */
    mv::Dataset<Volumes>     _volumeDataset;
    TrackballCamera          _camera;

    mv::Vector3f             _minClippingPlane = mv::Vector3f(0, 0, 0);
    mv::Vector3f             _maxClippingPlane = mv::Vector3f(1, 1, 1);

    QPointF                 _previousMousePos;  /* Previous mouse position */
    float                   _pixelRatio;        /* Pixel ratio */
    bool                    _mousePressed;      /* Whether the mouse is pressed */
    bool                    _isInitialized;     /* Whether OpenGL is initialized */
    bool                    _isNavigating;

};
