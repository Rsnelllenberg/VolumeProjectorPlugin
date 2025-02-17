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

struct gradientData {
    bool gradient;
    int textureID;
    float xOffset;
    float yOffset;
    float width;
    float height;
	int rotation;
};

class InteractiveShape {
public:
    InteractiveShape(const QPixmap& pixmap, const QRectF& rect, const QRect& bounds, QColor pixmapColor, qreal threshold = 10.0);

    void draw(QPainter& painter, bool drawBorder, bool normalizeWindow = true, QColor borderColor = Qt::black) const;
	void drawID(QPainter& painter, bool normalizeWindow, int id) const;
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

	void updateGradient(gradientData data);
	QImage getGradientImage() const;
	gradientData getGradientData() const;

	void updatePixmap();

private:
    QRectF getRelativeRect() const;
    QRectF getAbsoluteRect() const;

private:
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

	gradientData _gradientData;
};
