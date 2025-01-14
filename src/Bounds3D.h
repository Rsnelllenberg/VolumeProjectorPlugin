#pragma once

#include "graphics/Vector3f.h"

/**
    Lightweight rectangular bounds class.

    This bounds class:
        * Has a bottom-left anchor point.
        * Does not allow negative sizes
        * Does not secretly clamp values set by the user
*/
class Bounds3D
{
public:
    const static Bounds3D Max;

    Bounds3D();
    Bounds3D(const Bounds3D&);
    Bounds3D(float left, float right, float bottom, float top, float front, float back);

    void setBounds(float left, float right, float bottom, float top, float front, float back);
    void ensureMinimumSize(float width, float height, float depth);
    void moveToOrigin();
    void makeCube();
    void expand(float fraction);

    float getWidth() const { return _right - _left; }
    float getHeight() const { return _top - _bottom; }
    float getDepth() const { return _back - _front; }
    mv::Vector3f getCenter() const { return { (_left + _right) / 2, (_bottom + _top) / 2, (_front + _back) / 2 }; }

    float getLeft() const { return _left; }
    float getRight() const { return _right; }
    float getBottom() const { return _bottom; }
    float getTop() const { return _top; }
    float getFront() const { return _front; }
    float getBack() const { return _back; }

    void setLeft(float left) { _left = left; }
    void setRight(float right) { _right = right; }
    void setBottom(float bottom) { _bottom = bottom; }
    void setTop(float top) { _top = top; }
    void setFront(float front) { _front = front; }
    void setBack(float back) { _back = back; }

    friend bool operator==(const Bounds3D& lhs, const Bounds3D& rhs) {
        float eps = 0.0001f;
        return std::abs(lhs.getLeft() - rhs.getLeft()) < eps &&
            std::abs(lhs.getRight() - rhs.getRight()) < eps &&
            std::abs(lhs.getBottom() - rhs.getBottom()) < eps &&
            std::abs(lhs.getTop() - rhs.getTop()) < eps &&
            std::abs(lhs.getFront() - rhs.getFront()) < eps &&
            std::abs(lhs.getBack() - rhs.getBack()) < eps;
    }

    friend bool operator!=(const Bounds3D& lhs, const Bounds3D& rhs) {
        return !(lhs == rhs);
    }

private:
    float _left;
    float _right;
    float _bottom;
    float _top;
    float _front;
    float _back;
};
