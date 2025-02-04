#pragma once
#include <QColor>
#include <QPainter>
#include <QRectF>
#include <QPixmap>

class InteractiveShape {
public:
    InteractiveShape(const QPixmap& pixmap, const QRectF& rect)
        : _pixmap(pixmap), _rect(rect), _isSelected(false) {
    }

    void draw(QPainter& painter) const {
        painter.setBrush(_isSelected ? Qt::red : Qt::blue);
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

private:
    QPixmap _pixmap;
    QRectF _rect;
    bool _isSelected;
};
