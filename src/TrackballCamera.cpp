#include "TrackballCamera.h"

TrackballCamera::TrackballCamera()
    : _distance(100.0f), _angleX(0.0f), _angleY(0.0f), _viewportWidth(800), _viewportHeight(600), _center(0.0f, 0.0f, 0.0f)
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

void TrackballCamera::setCenter(const QVector3D& center)
{
    _center = center;
}

void TrackballCamera::mousePress(const QPointF& pos)
{
    _lastMousePos = pos;
}

// Change camera rotation depending on mouse position
void TrackballCamera::rotateCamera(const QPointF& pos)
{
    QPointF diff = pos - _lastMousePos;
    // Rotate the camera
    _angleY += diff.x() * 0.5f;
    _angleX += diff.y() * 0.5f;
    _lastMousePos = pos;
}

// Change Center depending on mouse position 
void TrackballCamera::shiftCenter(const QPointF& pos)
{

    QPointF diff = pos - _lastMousePos;
    // Move the center point
    _center.setX(_center.x() - diff.x() * 0.1f * _distance / _viewportWidth);
    _center.setY(_center.y() + diff.y() * 0.1f * _distance / _viewportHeight);
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
    view.translate(-_center);
    return view;
}


float TrackballCamera::getAspect() const
{
    return static_cast<float>(_viewportWidth) / _viewportHeight;
}
