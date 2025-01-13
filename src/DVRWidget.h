#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_2_Core>

#include "VolumeRenderer.h"
#include "VoxelBox.h"

#include "graphics/Vector3f.h"
#include "graphics/Vector2f.h"

#include <vector>
#include <QVector3D>
#include "TrackballCamera.h"
#include "DVRViewPlugin.h"


class DVRViewPlugin;

class DVRWidget : public QOpenGLWidget, QOpenGLFunctions_4_2_Core
{
    Q_OBJECT

public:
    DVRWidget();
    ~DVRWidget();

    void setData(std::vector<float>& spatialData, std::vector<float>& valueData, int valueDims); // convert the float lists to the correct format
    void paintGL()              Q_DECL_OVERRIDE;

    /** Returns true when the widget was initialized and is ready to be used. */
    bool isInitialized() const { return _isInitialized; };

signals:
    void initialized();

protected:
    void initializeGL()         Q_DECL_OVERRIDE;
    void resizeGL(int w, int h) Q_DECL_OVERRIDE;
    void cleanup();
    bool eventFilter(QObject* target, QEvent* event);

private:
    VolumeRenderer _volumeRenderer;
    bool _isInitialized = false;

    TrackballCamera _camera;
    bool _mousePressed;
    QPointF _previousMousePos;

    QColor                  _backgroundColor;   /* Background color */
};
