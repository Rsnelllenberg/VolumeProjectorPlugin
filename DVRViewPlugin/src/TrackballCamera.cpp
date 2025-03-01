#include "TrackballCamera.h"

TrackballCamera::TrackballCamera()
    : _distance(100.0f), _rotation(), _viewportWidth(800), _viewportHeight(600), _center(0.0f, 0.0f, 0.0f)
{
}

void TrackballCamera::setDistance(float distance)
{
    _distance = distance;
}

void TrackballCamera::setRotation(float angleX, float angleY)
{
    _rotation = QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), angleX) * QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), angleY);
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

QVector3D TrackballCamera::up() const
{
    return _rotation * QVector3D(0, 1, 0);
}

QVector3D TrackballCamera::left() const
{
    return _rotation * QVector3D(-1, 0, 0);
}


// Change camera rotation depending on mouse position
void TrackballCamera::rotateCamera(const QPointF& pos)
{
    QPointF diff = pos - _lastMousePos;

    // Rotate the camera
    _rotation = QQuaternion::fromAxisAndAngle(up(), -diff.x() * 0.5f) * _rotation;
    _rotation = QQuaternion::fromAxisAndAngle(left(), diff.y() * 0.5f) * _rotation;
    _rotation.normalize(); // To avoid drifting over time

    _lastMousePos = pos;
}

// Change Center depending on mouse position 
void TrackballCamera::shiftCenter(const QPointF& pos)
{
    QPointF diff = pos - _lastMousePos;

    // Calculate the movement based on the camera's current orientation
    _center += diff.x() * 0.1f * _distance / _viewportWidth * left();
    _center += diff.y() * 0.1f * _distance / _viewportHeight * up();

    _lastMousePos = pos;
}

void TrackballCamera::mouseWheel(float delta)
{
    _distance -= delta * 0.1f;
    if (_distance < 1.0f) _distance = 1.0f; // Prevent the camera from getting too close
}

QMatrix4x4 TrackballCamera::getViewMatrix() const
{
    QMatrix4x4 viewMatrix;
    QVector3D cameraPos = _center + _rotation * QVector3D(0, 0, _distance);
    viewMatrix.lookAt(cameraPos, _center, up());

    return viewMatrix;
}

QMatrix4x4 TrackballCamera::getProjectionMatrix() const
{
    QMatrix4x4 projectionMatrix;
    projectionMatrix.perspective(45.0f, getAspect(), 10.0f, 20000.0f);
    return projectionMatrix;
}

QVector3D TrackballCamera::getPosition() const
{
    return _center + _rotation * QVector3D(0, 0, _distance);
}


float TrackballCamera::getAspect() const
{
    return static_cast<float>(_viewportWidth) / _viewportHeight;
}
