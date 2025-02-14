#include "InteractiveShape.h"
#include <QDebug>

InteractiveShape::InteractiveShape(const QPixmap& pixmap, const QRectF& rect, const QRect& bounds, QColor pixmapColor, qreal threshold)
    : _pixmap(pixmap), _rect(rect), _bounds(bounds), _isSelected(false), _pixmapColor(pixmapColor), _threshold(threshold) {
    _mask = _pixmap.createMaskFromColor(Qt::transparent);

    _gradient1D = QImage(":textures/gaussian1D_texture", ".png");
    _gradient2D = QImage(":textures/gaussian_texture", ".png");
	//_gradient1D.scaled(_pixmap.size());
	//_gradient2D.scaled(_pixmap.size());
}

void InteractiveShape::draw(QPainter& painter, bool drawBorder, bool normalizeWindow, QColor borderColor) const {
    QRectF adjustedRect;
    if (normalizeWindow) {
        adjustedRect = getRelativeRect();
    } else {
        adjustedRect = getAbsoluteRect();
    }
    if (drawBorder) {
        QPen pen(borderColor);
        pen.setWidth(2);
        painter.setPen(pen);
        painter.drawRect(adjustedRect);

        QRectF topRightRect(adjustedRect.topRight() - QPointF(_threshold, 0), QSizeF(_threshold, _threshold));
        painter.setPen(pen);
        painter.drawRect(topRightRect);
    }

    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawPixmap(adjustedRect.toRect(), _pixmap);
}

bool InteractiveShape::contains(const QPointF& point) const {
    return getRelativeRect().contains(point);
}

void InteractiveShape::moveBy(const QPointF& delta) {
    _rect.translate(delta.x() / _bounds.width(), delta.y() / _bounds.height());
}

void InteractiveShape::resizeBy(const QPointF& delta, SelectedSide& side) {
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
        } else {
            side = SelectedSide::Left;
        }
    } else if (_rect.top() > _rect.bottom()) {
        qreal temp = _rect.top();
        _rect.setTop(_rect.bottom());
        _rect.setBottom(temp);
        if (side == SelectedSide::Top) {
            side = SelectedSide::Bottom;
        } else {
            side = SelectedSide::Top;
        }
    }
}

bool InteractiveShape::isNearSide(const QPointF& point, SelectedSide& side) const {
    QRectF adjustedRect = getRelativeRect();
    if (std::abs(point.x() - adjustedRect.left()) <= _threshold && point.y() <= adjustedRect.bottom() && point.y() >= adjustedRect.top()) {
        side = SelectedSide::Left;
        return true;
    } else if (std::abs(point.x() - adjustedRect.right()) <= _threshold && point.y() <= adjustedRect.bottom() && point.y() >= adjustedRect.top()) {
        side = SelectedSide::Right;
        return true;
    } else if (std::abs(point.y() - adjustedRect.top()) <= _threshold && point.x() <= adjustedRect.right() && point.x() >= adjustedRect.left()) {
        side = SelectedSide::Top;
        return true;
    } else if (std::abs(point.y() - adjustedRect.bottom()) <= _threshold && point.x() <= adjustedRect.right() && point.x() >= adjustedRect.left()) {
        side = SelectedSide::Bottom;
        return true;
    }
    side = SelectedSide::None;
    return false;
}

void InteractiveShape::setSelected(bool selected) {
    _isSelected = selected;
}

bool InteractiveShape::getSelected() const {
    return _isSelected;
}

void InteractiveShape::setColor(const QColor& color) {


    QPixmap newPixmap(_pixmap.size());
    newPixmap.fill(Qt::transparent);

    QPainter painter(&newPixmap);
    painter.setClipRegion(QRegion(_mask));
    painter.fillRect(newPixmap.rect(), color);
    painter.end();

    _colormap = newPixmap;
    _pixmapColor = color;

    updatePixmap();
}

QColor InteractiveShape::getColor() const {
    return _pixmapColor;
}

bool InteractiveShape::isNearTopRightCorner(const QPointF& point) const {
    QPointF topRight = getRelativeRect().topRight();
    return (std::abs(point.x() - topRight.x()) <= _threshold && std::abs(point.y() - topRight.y()) <= _threshold);
}

void InteractiveShape::setThreshold(qreal threshold) {
    _threshold = threshold;
}

void InteractiveShape::setBounds(const QRect& bounds) {
    _bounds = bounds;
}

void InteractiveShape::updateGradient(float xOffset, float yOffset, float width, float height, int textureID)
{
	QImage gradient;
	if (textureID == 0)
		gradient = _gradient1D;
	else if (textureID == 1)
		gradient = _gradient2D;
    else {
        qCritical() << "Unknown texture ID: currently only 0 and 1 exist";
        return;
    }


	gradient = gradient.copy(QRect((xOffset + ((1 - width) / 2)) * gradient.width(), (yOffset + ((1 - height) / 2)) * gradient.height(), width * gradient.width(), height * gradient.height()));
    gradient.invertPixels(QImage::InvertRgb);
    gradient = gradient.transformed(QTransform().rotate(30));
    gradient.scaled(_pixmap.size());

	_usedGradient = gradient;

	updatePixmap();
}

void InteractiveShape::updatePixmap()
{
	if (_colormap.isNull())
		return;

    QImage pixmapImage = _colormap.toImage();
    if (!_usedGradient.isNull())
        pixmapImage.setAlphaChannel(_usedGradient);

    _pixmap = QPixmap::fromImage(pixmapImage);
}

QRectF InteractiveShape::getRelativeRect() const {
    return QRectF(
        _bounds.left() + _rect.left() * _bounds.width(),
        _bounds.top() + _rect.top() * _bounds.height(),
        _rect.width() * _bounds.width(),
        _rect.height() * _bounds.height()
    );
}

QRectF InteractiveShape::getAbsoluteRect() const {
    return QRectF(
        _rect.left() * _bounds.width(),
        _rect.top() * _bounds.height(),
        _rect.width() * _bounds.width(),
        _rect.height() * _bounds.height()
    );
}
