#pragma once
#include <QColor>
#include <QPainter>
#include <QRectF>
#include <QPixmap>

enum class SelectedSide {
    None,
    Left,
    Right,
    Top,
    Bottom
};

class InteractiveShape {
public:
    InteractiveShape(const QPixmap& pixmap, const QRectF& rect, const QRect& bounds, qreal threshold = 10.0)
        : _pixmap(pixmap), _rect(rect), _bounds(bounds), _isSelected(false), _threshold(threshold) {
    }

	// Draw the shape, with or without border, default border color is black. The normalizeWindow parameter should be used when the window size isn't the same as the size of the point bounds
    void draw(QPainter& painter, bool drawBorder, bool normalizeWindow = true, QColor borderColor = Qt::black) const {
        QRectF adjustedRect;
		if (normalizeWindow) {
			adjustedRect = getRelativeRect();
		}
		else {
			adjustedRect = getAbsoluteRect();
		}
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

    void resizeBy(const QPointF& delta, SelectedSide& side) {
        switch (side) {
        case SelectedSide::Left:
            _rect.setLeft(_rect.left() + delta.x() / _bounds.width());
            break;
        case SelectedSide::Right:
            _rect.setRight(_rect.right() + delta.x() / _bounds.width());
            break;
        case SelectedSide::Top:
            _rect.setTop(_rect.top() + delta.y() / _bounds.height());
            break;
        case SelectedSide::Bottom:
            _rect.setBottom(_rect.bottom() + delta.y() / _bounds.height());
            break;
        default:
            break;
        }
        if (_rect.left() > _rect.right()) {
			qreal temp = _rect.left();
			_rect.setLeft(_rect.right());
			_rect.setRight(temp);
			if (side == SelectedSide::Left) {
				side = SelectedSide::Right;
			}
			else {
				side = SelectedSide::Left;
			}
		}
        else if (_rect.top() > _rect.bottom()) {
            qreal temp = _rect.top();
            _rect.setTop(_rect.bottom());
            _rect.setBottom(temp);
			if (side == SelectedSide::Top) {
				side = SelectedSide::Bottom;
			}
			else {
				side = SelectedSide::Top;
			}
        }
			
    }

    bool isNearSide(const QPointF& point, SelectedSide& side) const {
        QRectF adjustedRect = getRelativeRect();
        if (std::abs(point.x() - adjustedRect.left()) <= _threshold && point.y() <= adjustedRect.bottom() && point.y() >= adjustedRect.top()) {
			side = SelectedSide::Left;
            return true;
        }
		else if (std::abs(point.x() - adjustedRect.right()) <= _threshold && point.y() <= adjustedRect.bottom() && point.y() >= adjustedRect.top()) {
			side = SelectedSide::Right;
			return true;
        }
		else if (std::abs(point.y() - adjustedRect.top()) <= _threshold && point.x() <= adjustedRect.right() && point.x() >= adjustedRect.left()) {
			side = SelectedSide::Top;
			return true;
        }
		else if (std::abs(point.y() - adjustedRect.bottom()) <= _threshold && point.x() <= adjustedRect.right() && point.x() >= adjustedRect.left()) {
			side = SelectedSide::Bottom;
			return true;
        }
		side = SelectedSide::None;
		return false;
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

    bool isNearSide(const QPointF& point, QString& side) const {
        QRectF adjustedRect = getRelativeRect();
        if (std::abs(point.x() - adjustedRect.left()) <= _threshold) {
            side = "left";
            return true;
        }
        else if (std::abs(point.x() - adjustedRect.right()) <= _threshold) {
            side = "right";
            return true;
        }
        else if (std::abs(point.y() - adjustedRect.top()) <= _threshold) {
            side = "top";
            return true;
        }
        else if (std::abs(point.y() - adjustedRect.bottom()) <= _threshold) {
            side = "bottom";
            return true;
        }
        return false;
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

    QRectF getAbsoluteRect() const {
        return QRectF(
            _rect.left() * _bounds.width(),
            _rect.top() * _bounds.height(),
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
