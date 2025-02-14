#pragma once
#include <QColor>
#include <QPainter>
#include <QRectF>
#include <QPixmap>
#include <QBitmap>

enum class SelectedSide {
    None,
    Left,
    Right,
    Top,
    Bottom
};

class InteractiveShape {
public:
    InteractiveShape(const QPixmap& pixmap, const QRectF& rect, const QRect& bounds, QColor pixmapColor, qreal threshold = 10.0);

    void draw(QPainter& painter, bool drawBorder, bool normalizeWindow = true, QColor borderColor = Qt::black) const;
    bool contains(const QPointF& point) const;
    void moveBy(const QPointF& delta);
    void resizeBy(const QPointF& delta, SelectedSide& side);

    bool isNearSide(const QPointF& point, SelectedSide& side) const;
    bool isNearTopRightCorner(const QPointF& point) const;

    void setSelected(bool selected);
    bool getSelected() const;

    void setColor(const QColor& color);
    QColor getColor() const;

    void setThreshold(qreal threshold);
    void setBounds(const QRect& bounds);

	void updateGradient(float xOffset, float yOffset, float width, float height, int textureID);
	void updatePixmap();

private:
    QRectF getRelativeRect() const;
    QRectF getAbsoluteRect() const;

    QPixmap _pixmap;
    QPixmap _colormap;
    QRectF _rect;
    QRect _bounds;
    bool _isSelected;
    qreal _threshold;
    QColor _pixmapColor;
    QBitmap _mask;
	QImage _gradient1D;
	QImage _gradient2D;
    QImage _usedGradient;
};
