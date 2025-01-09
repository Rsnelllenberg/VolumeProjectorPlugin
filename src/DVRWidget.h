#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_2_Core>

#include "VolumeRenderer.h"

#include "graphics/Vector3f.h"
#include "graphics/Vector2f.h"

#include <vector>

/**
 * OpenGL Volume Renderer Widget
 * This class provides a widget interface to the OpenGL Volume Renderer
 *
 * @author Julian Thijssen
 */
class DVRWidget : public QOpenGLWidget, QOpenGLFunctions_4_2_Core
{
    Q_OBJECT

public:
    DVRWidget();

    void setTexels(int width, int height, int depth, std::vector<float>& texels);
    void setData(std::vector<float>& data);
    void setColors(std::vector<float>& colors);
    void setColormap(const QImage& colormap);
    void setCursorPoint(mv::Vector3f cursorPoint);

public:
    bool eventFilter(QObject* target, QEvent* event);

signals:
    void initialized();

protected:
    void initializeGL()         Q_DECL_OVERRIDE;
    void resizeGL(int w, int h) Q_DECL_OVERRIDE;
    void paintGL()              Q_DECL_OVERRIDE;
    void cleanup();

private:
    VolumeRenderer _volumeRenderer;

    mv::Vector3f _camPos;
    float _camDist = 1.0f;
    mv::Vector2f _camAngle = mv::Vector2f(3.14159f / 2, 0);

    QPointF _previousMousePos;
    bool _mousePressed = false;

    bool _isInitialized = false;
};
