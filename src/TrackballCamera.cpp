#include "TrackballCamera.h"

TrackballCamera::TrackballCamera()
    : _distance(5.0f), _angleX(0.0f), _angleY(0.0f), _viewportWidth(800), _viewportHeight(600)
{
}

void TrackballCamera::setDistance(float distance)
{
    _distance = distance;
}

void TrackballCamera::setRotation(float angleX, float angleY)
{
    _angleX = angleX;
    _angleY = angleY;
}

void TrackballCamera::setViewport(int width, int height)
{
    _viewportWidth = width;
    _viewportHeight = height;
}

void TrackballCamera::mousePress(const QPointF& pos)
{
    _lastMousePos = pos;
}

void TrackballCamera::mouseMove(const QPointF& pos)
{
    QPointF diff = pos - _lastMousePos;
    _angleY += diff.x() * 0.5f;
    _angleX += diff.y() * 0.5f;
    _lastMousePos = pos;
}

void TrackballCamera::mouseWheel(float delta)
{
    _distance -= delta * 0.1f;
    if (_distance < 1.0f) _distance = 1.0f; // Prevent the camera from getting too close
}

QMatrix4x4 TrackballCamera::getViewMatrix() const
{
    QMatrix4x4 view;
    view.translate(0, 0, -_distance);
    view.rotate(_angleX, 1, 0, 0);
    view.rotate(_angleY, 0, 1, 0);
    return view;
}

float TrackballCamera::getAspect() const
{
    return static_cast<float>(_viewportWidth) / _viewportHeight;
}
