#pragma once
#include <QColor>
#include <QPainter>
#include <QRectF>

class InteractiveShape {
public:
    InteractiveShape(const QRectF& rect, const QColor& color)
        : _rect(rect), _color(color), _isSelected(false) {}

    void draw(QPainter& painter) const {
        painter.setBrush(_isSelected ? Qt::red : _color);
        painter.drawRect(_rect);
		painter.fillRect(_rect, _color);
		qDebug() << "Drawing rectangle";
    }

    bool contains(const QPointF& point) const {
        return _rect.contains(point);
    }

    void moveBy(const QPointF& delta) {
        _rect.translate(delta);
    }

    void setSelected(bool selected) {
        _isSelected = selected;
    }

private:
    QRectF _rect;
    QColor _color;
    bool _isSelected;
};
