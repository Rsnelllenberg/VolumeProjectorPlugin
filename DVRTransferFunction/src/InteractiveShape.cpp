#include "InteractiveShape.h"
#include <QDebug>

InteractiveShape::InteractiveShape(const QPixmap& pixmap, const QRectF& rect, const QRect& bounds, QColor pixmapColor, float globalAlphaValue, qreal threshold)
	: _pixmap(pixmap), _rect(rect), _bounds(bounds), _isSelected(false), _pixmapColor(pixmapColor), _globalAlphaValue(globalAlphaValue), _threshold(threshold)  {
    _mask = _pixmap.createMaskFromColor(Qt::transparent);

    _gradient1D = QImage(":textures/gaussian1D_texture", ".png");
    _gradient2D = QImage(":textures/gaussian_texture", ".png");
	//_gradient1D.scaled(_pixmap.size());
	//_gradient2D.scaled(_pixmap.size());

	_gradientData = { false, 0, 0.0f, 0.0f, 1.0f, 1.0f, 0 };

	setGlobalAlphaValue(globalAlphaValue); // This will create the globalAlphaPixmap
}

void InteractiveShape::draw(QPainter& painter, bool drawBorder, bool useGlobalAlpha, bool normalizeWindow /*true*/, QColor borderColor /* Black */) const {
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
	if (useGlobalAlpha) {
		painter.drawPixmap(adjustedRect.toRect(), _globalAlphaColormap);
	}
    else {
        painter.drawPixmap(adjustedRect.toRect(), _pixmap);
    }
}

void InteractiveShape::drawID(QPainter& painter, bool normalizeWindow, int id) const {
    QRectF adjustedRect;
    if (normalizeWindow) {
        adjustedRect = getRelativeRect();
    }
    else {
        adjustedRect = getAbsoluteRect();
    }

    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    
    QPixmap newPixmap(_pixmap.size());
    newPixmap.fill(Qt::transparent);

    QPainter tempPainter(&newPixmap);
    tempPainter.setClipRegion(QRegion(_mask));
	tempPainter.fillRect(newPixmap.rect(), QColor(id, id, id));
    tempPainter.end();

    painter.drawPixmap(adjustedRect.toRect(), newPixmap);
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

    _pixmapColor = color;

    QPixmap newPixmap(_pixmap.size());
    newPixmap.fill(Qt::transparent);

    QPainter painter(&newPixmap);
    painter.setClipRegion(QRegion(_mask));
    painter.fillRect(newPixmap.rect(), color);

    _colormap = newPixmap;

	QColor globalColor = _pixmapColor;
    globalColor.setAlpha(_globalAlphaValue);
    painter.fillRect(newPixmap.rect(), globalColor);
    painter.end();

	_globalAlphaColormap = newPixmap;

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

void InteractiveShape::updateGradient(gradientData data)
{
	_gradientData = data;
    if (_gradientData.gradient) {
        QImage gradient;
        if (_gradientData.textureID == 0)
            gradient = _gradient1D;
        else if (_gradientData.textureID == 1)
            gradient = _gradient2D;
        else {
            qCritical() << "Unknown texture ID: currently only 0 and 1 exist";
            return;
        }
		QSize pixmapSize = _pixmap.size();
        float biggestFit = std::max(gradient.width() / pixmapSize.width(), gradient.height() / pixmapSize.height());

        gradient = gradient.copy(QRect((_gradientData.xOffset + ((1 - _gradientData.width) / 2)) * gradient.width(), (_gradientData.yOffset + ((1 - _gradientData.height) / 2)) * gradient.height(), _gradientData.width * gradient.width(), _gradientData.height * gradient.height()));
		gradient.scaled(_pixmap.size() * biggestFit); //scale to the ratio of the pixmap to simulate gradient width and height

        gradient = gradient.transformed(QTransform().rotate(_gradientData.rotation));
        //gradient = gradient.copy(QRect((gradient.width() - _pixmap.width()) / 2, (gradient.height() - _pixmap.height()) / 2, _pixmap.width(), _pixmap.height()));
        //gradient.invertPixels(QImage::InvertRgb);

        _usedGradient = gradient;
    }
    updatePixmap();
}

QImage InteractiveShape::getGradientImage() const
{
    return _usedGradient;
}

gradientData InteractiveShape::getGradientData() const
{
    return _gradientData;
}

void InteractiveShape::setGlobalAlphaValue(int globalAlphaValue)
{
	_globalAlphaValue = globalAlphaValue;

	//Update _globalAlphaColormap
	QColor globalColor = _pixmapColor;
	globalColor.setAlpha(_globalAlphaValue);

	QPixmap newPixmap(_pixmap.size());
	newPixmap.fill(Qt::transparent);

	QPainter painter(&newPixmap);
	painter.setClipRegion(QRegion(_mask));
	painter.fillRect(newPixmap.rect(), globalColor);
	painter.end();

	_globalAlphaColormap = newPixmap;
	updatePixmap();
}

void InteractiveShape::updatePixmap()
{
	if (_colormap.isNull())
		return;

    if (_usedGradient.isNull() || !_gradientData.gradient) {
        _pixmap = _colormap;
		_globalAlphaPixmap = _globalAlphaColormap;
    }
    else {
        QImage pixmapImage = _colormap.toImage();
        pixmapImage.setAlphaChannel(_usedGradient);
        _pixmap = QPixmap::fromImage(pixmapImage);

		QImage globalAlphaImage = _globalAlphaColormap.toImage();
		globalAlphaImage.setAlphaChannel(_usedGradient);
		_globalAlphaPixmap = QPixmap::fromImage(globalAlphaImage);
    }
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
