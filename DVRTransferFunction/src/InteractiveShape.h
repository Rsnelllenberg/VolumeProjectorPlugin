// InteractiveShape.h
#pragma once
#include <QColor>
#include <QPainter>
#include <QRectF>
#include <QPixmap>

class InteractiveShape {
public:
    InteractiveShape(const QPixmap& pixmap, const QRectF& rect, qreal threshold = 10.0)
        : _pixmap(pixmap), _rect(rect), _isSelected(false), _threshold(threshold) {
    }

    // Draw the shape, with or without border, default border color is black
    void draw(QPainter& painter, bool drawBorder, QColor borderColor = Qt::black) const {
        if (drawBorder) {
            // Draw border around the rectangle outline
            QPen pen(borderColor);
            pen.setWidth(2);
            painter.setPen(pen);
            painter.drawRect(_rect);

            // Draw small rectangle at the top right corner to indicate the threshold area
            QRectF topRightRect(_rect.topRight() - QPointF(_threshold, 0), QSizeF(_threshold, _threshold));
            painter.setPen(pen);
            painter.drawRect(topRightRect);
        }

        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.drawPixmap(_rect.toRect(), _pixmap);


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

    bool getSelected() const {
        return _isSelected;
    }

    bool isNearTopRightCorner(const QPointF& point) const {
        QPointF topRight = _rect.topRight();
        return (std::abs(point.x() - topRight.x()) <= _threshold && std::abs(point.y() - topRight.y()) <= _threshold);
    }

    void setThreshold(qreal threshold) {
        _threshold = threshold;
    }

private:
    QPixmap _pixmap;
    QRectF _rect;
    bool _isSelected;
    qreal _threshold;
};
