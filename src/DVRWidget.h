#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_4_3_Core>

#include "VolumeRenderer.h"
#include "VoxelBox.h"

#include "graphics/Vector3f.h"
#include "graphics/Vector2f.h"

#include <vector>
#include <QVector3D>
#include "TrackballCamera.h"
#include "DVRViewPlugin.h"


class DVRViewPlugin;

class DVRWidget : public QOpenGLWidget, QOpenGLFunctions_4_3_Core
{
    Q_OBJECT

public:
    DVRWidget();
    ~DVRWidget();

    void setData(std::vector<float>& spatialData, std::vector<float>& valueData, int valueDims); // convert the float lists to the correct format

    /** Returns true when the widget was initialized and is ready to be used. */
    bool isInitialized() const { return _isInitialized; };

signals:
    void initialized();

protected:
    void initializeGL()         override;
    void resizeGL(int w, int h) override;
    void paintGL()              override;
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
