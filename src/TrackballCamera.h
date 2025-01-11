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
    void mouseMove(const QPointF& pos, bool isRightButton);
    void mouseWheel(float delta);

    QMatrix4x4 getViewMatrix() const;
    float getAspect() const;

private:
    float _distance;
    float _angleX;
    float _angleY;
    int _viewportWidth;
    int _viewportHeight;
    QPointF _lastMousePos;
    QVector3D _center;
};

#endif // TRACKBALLCAMERA_H
