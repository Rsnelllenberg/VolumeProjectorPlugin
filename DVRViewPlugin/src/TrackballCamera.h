#ifndef TRACKBALLCAMERA_H
#define TRACKBALLCAMERA_H

#include <QVector3D>
#include <QMatrix4x4>
#include <QPointF>

class TrackballCamera
{
public:
    TrackballCamera();

    void setDistance(float distance);
    void setRotation(float angleX, float angleY);
    void setViewport(int width, int height);
    void setCenter(const QVector3D& center);

    void mousePress(const QPointF& pos);
    QVector3D up() const;
    QVector3D left() const;
    void rotateCamera(const QPointF& pos);
    void shiftCenter(const QPointF& pos);
    void mouseWheel(float delta);

    QMatrix4x4 getViewMatrix() const;
    QMatrix4x4 getProjectionMatrix() const;
    QVector3D getPosition() const;
    float getAspect() const;

private:
    float _distance;
    QQuaternion _rotation;
    int _viewportWidth;
    int _viewportHeight;
    QPointF _lastMousePos;
    QVector3D _center;
};

#endif // TRACKBALLCAMERA_H
