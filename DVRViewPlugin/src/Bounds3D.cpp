#include "Bounds3D.h"

#include <limits>

const Bounds3D Bounds3D::Max = Bounds3D(
    std::numeric_limits<float>::infinity(),
    -std::numeric_limits<float>::infinity(),
    std::numeric_limits<float>::infinity(),
    -std::numeric_limits<float>::infinity(),
    std::numeric_limits<float>::infinity(),
    -std::numeric_limits<float>::infinity()
);

Bounds3D::Bounds3D() :
    Bounds3D(-1, 1, -1, 1, -1, 1)
{
}

Bounds3D::Bounds3D(float left, float right, float bottom, float top, float front, float back) :
    _left(left),
    _right(right),
    _bottom(bottom),
    _top(top),
    _front(front),
    _back(back)
{
}

Bounds3D::Bounds3D(const Bounds3D& b) :
    _left(b._left),
    _right(b._right),
    _bottom(b._bottom),
    _top(b._top),
    _front(b._front),
    _back(b._back)
{
}

void Bounds3D::setBounds(float left, float right, float bottom, float top, float front, float back)
{
    _left = left;
    _right = right;
    _bottom = bottom;
    _top = top;
    _front = front;
    _back = back;
}

void Bounds3D::ensureMinimumSize(float width, float height, float depth)
{
    mv::Vector3f center = getCenter();

    if (getWidth() < width)
    {
        _left = center.x - width / 2;
        _right = center.x + width / 2;
    }
    if (getHeight() < height)
    {
        _bottom = center.y - height / 2;
        _top = center.y + height / 2;
    }
    if (getDepth() < depth)
    {
        _front = center.z - depth / 2;
        _back = center.z + depth / 2;
    }
}

void Bounds3D::moveToOrigin()
{
    mv::Vector3f center = getCenter();
    _left -= center.x;
    _right -= center.x;
    _bottom -= center.y;
    _top -= center.y;
    _front -= center.z;
    _back -= center.z;
}

void Bounds3D::makeCube()
{
    mv::Vector3f center = getCenter();
    float halfSize = std::max({ getWidth(), getHeight(), getDepth() }) / 2;

    _left = center.x - halfSize;
    _right = center.x + halfSize;
    _bottom = center.y - halfSize;
    _top = center.y + halfSize;
    _front = center.z - halfSize;
    _back = center.z + halfSize;
}

void Bounds3D::expand(float fraction)
{
    float widthOffset = (getWidth() * fraction) / 2;
    float heightOffset = (getHeight() * fraction) / 2;
    float depthOffset = (getDepth() * fraction) / 2;

    _left -= widthOffset;
    _right += widthOffset;
    _bottom -= heightOffset;
    _top += heightOffset;
    _front -= depthOffset;
    _back += depthOffset;
}

