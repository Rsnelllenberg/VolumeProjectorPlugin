#pragma once
#include <QColor>
#include <QPainter>
#include <QRectF>
#include <QPixmap>

class InteractiveShape {
public:
    InteractiveShape(const QPixmap& pixmap, const QRectF& rect, const QRect& bounds, qreal threshold = 10.0)
        : _pixmap(pixmap), _rect(rect), _bounds(bounds), _isSelected(false), _threshold(threshold) {
    }

    // Draw the shape, with or without border, default border color is black
    void draw(QPainter& painter, bool drawBorder, QColor borderColor = Qt::black) const {
        QRectF adjustedRect = getRelativeRect();
        if (drawBorder) {
            // Draw border around the rectangle outline
            QPen pen(borderColor);
            pen.setWidth(2);
            painter.setPen(pen);
            painter.drawRect(adjustedRect);

            // Draw small rectangle at the top right corner to indicate the threshold area
            QRectF topRightRect(adjustedRect.topRight() - QPointF(_threshold, 0), QSizeF(_threshold, _threshold));
            painter.setPen(pen);
            painter.drawRect(topRightRect);
        }

        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.drawPixmap(adjustedRect.toRect(), _pixmap);
    }

    bool contains(const QPointF& point) const {
        return getRelativeRect().contains(point);
    }

    void moveBy(const QPointF& delta) {
        _rect.translate(delta.x() / _bounds.width(), delta.y() / _bounds.height());
    }

    void setSelected(bool selected) {
        _isSelected = selected;
    }

    bool getSelected() const {
        return _isSelected;
    }

    bool isNearTopRightCorner(const QPointF& point) const {
        QPointF topRight = getRelativeRect().topRight();
        return (std::abs(point.x() - topRight.x()) <= _threshold && std::abs(point.y() - topRight.y()) <= _threshold);
    }

    void setThreshold(qreal threshold) {
        _threshold = threshold;
    }

    void setBounds(const QRect& bounds) {
        _bounds = bounds;
    }

private:
    QRectF getRelativeRect() const {
        return QRectF(
            _bounds.left() + _rect.left() * _bounds.width(),
            _bounds.top() + _rect.top() * _bounds.height(),
            _rect.width() * _bounds.width(),
            _rect.height() * _bounds.height()
        );
    }

    QPixmap _pixmap;
    QRectF _rect;
    QRect _bounds;
    bool _isSelected;
    qreal _threshold;
};
