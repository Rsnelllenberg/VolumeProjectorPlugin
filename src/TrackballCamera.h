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

    void mousePress(const QPointF& pos);
    void mouseMove(const QPointF& pos);
    void mouseWheel(float delta);

    QMatrix4x4 getViewMatrix() const;
    float getAspect() const; // Add this line

private:
    float _distance;
    float _angleX;
    float _angleY;
    int _viewportWidth;
    int _viewportHeight;
    QPointF _lastMousePos;
};

#endif // TRACKBALLCAMERA_H
