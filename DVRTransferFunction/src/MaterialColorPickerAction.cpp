#include "MaterialColorPickerAction.h"
#include "TransferFunctionWidget.h"
#include "TransferFunctionPlugin.h"
#include "Application.h"

#include <QDebug>
#include <QHBoxLayout>
#include "InteractiveShape.h"

using namespace mv::gui;

const QColor MaterialColorPickerAction::DEFAULT_COLOR = Qt::gray;

MaterialColorPickerAction::MaterialColorPickerAction(QObject* parent, const QString& title, const QColor& color /*= DEFAULT_COLOR*/) :
    WidgetAction(parent, title),
    _color(color)
{
    setText(title);
    setColor(color);
}

QColor MaterialColorPickerAction::getColor() const
{
    return _color;
}

void MaterialColorPickerAction::setColor(const QColor& color)
{
    if (color == _color)
        return;

    _color = color;

    emit colorChanged(_color);
}

void MaterialColorPickerAction::initialize(TransferFunctionPlugin* transferFunctionPlugin)
{
    Q_ASSERT(transferFunctionPlugin != nullptr);

    if (transferFunctionPlugin == nullptr)
        return;

    TransferFunctionWidget& widget = transferFunctionPlugin->getTransferFunctionWidget();

    connect(&widget, &TransferFunctionWidget::shapeSelected, this, [this](InteractiveShape* shape) {
        qDebug() << "Shape selected";
        if (shape == nullptr)
            return;
        setColor(shape->getColor());
        });

    connect(this, &MaterialColorPickerAction::colorChanged, &widget, [this, &widget](const QColor& color) {
        InteractiveShape* shape = widget.getSelectedObject();
        if (shape == nullptr)
            return;
        shape->setColor(color);
        widget.update();
        });
}

void MaterialColorPickerAction::connectToPublicAction(WidgetAction* publicAction, bool recursive)
{
    auto publicColorPickerAction = dynamic_cast<MaterialColorPickerAction*>(publicAction);

    Q_ASSERT(publicColorPickerAction != nullptr);

    if (publicColorPickerAction == nullptr)
        return;

    connect(this, &MaterialColorPickerAction::colorChanged, publicColorPickerAction, &MaterialColorPickerAction::setColor);
    connect(publicColorPickerAction, &MaterialColorPickerAction::colorChanged, this, &MaterialColorPickerAction::setColor);

    setColor(publicColorPickerAction->getColor());

    WidgetAction::connectToPublicAction(publicAction, recursive);
}

void MaterialColorPickerAction::disconnectFromPublicAction(bool recursive)
{
    if (!isConnected())
        return;

    auto publicColorPickerAction = dynamic_cast<MaterialColorPickerAction*>(getPublicAction());

    Q_ASSERT(publicColorPickerAction != nullptr);

    if (publicColorPickerAction == nullptr)
        return;

    disconnect(this, &MaterialColorPickerAction::colorChanged, publicColorPickerAction, &MaterialColorPickerAction::setColor);
    disconnect(publicColorPickerAction, &MaterialColorPickerAction::colorChanged, this, &MaterialColorPickerAction::setColor);

    WidgetAction::disconnectFromPublicAction(recursive);
}

MaterialColorPickerAction::Widget::Widget(QWidget* parent, MaterialColorPickerAction* colorPickerAction) :
    WidgetActionWidget(parent, colorPickerAction),
    _layout(),
    _colorDialog(),
    _hueAction(this, "Hue", 0, 359, colorPickerAction->getColor().hue()),
    _saturationAction(this, "Saturation", 0, 255, colorPickerAction->getColor().hslSaturation()),
    _lightnessAction(this, "Lightness", 0, 255, colorPickerAction->getColor().lightness()),
    _alphaAction(this, "Alpha", 1, 255, colorPickerAction->getColor().alpha()),
    _updateColorPickerAction(true)
{
    setAcceptDrops(true);

    _colorDialog.setCurrentColor(colorPickerAction->getColor());
    _colorDialog.setOption(QColorDialog::ShowAlphaChannel, true);

    const auto getWidgetFromColorDialog = [this](const QString& name) -> QWidget* {
        auto allChildWidgets = _colorDialog.findChildren<QWidget*>();

        for (auto widget : allChildWidgets) {
            if (strcmp(widget->metaObject()->className(), name.toLatin1()) == 0)
                return widget;
        }

        return nullptr;
        };

    auto colorPickerWidget = getWidgetFromColorDialog("QColorPicker");
    auto colorLuminanceWidget = getWidgetFromColorDialog("QColorLuminancePicker");

    if (colorPickerWidget == nullptr)
        colorPickerWidget = getWidgetFromColorDialog("QtPrivate::QColorPicker");
    if (colorLuminanceWidget == nullptr)
        colorLuminanceWidget = getWidgetFromColorDialog("QtPrivate::QColorLuminancePicker");

    auto pickersLayout = new QHBoxLayout();

    pickersLayout->addWidget(colorPickerWidget);
    pickersLayout->addWidget(colorLuminanceWidget);

    // Set maximum height for pickersLayout
    auto pickersWidget = new QWidget();
    pickersWidget->setLayout(pickersLayout);
    pickersWidget->setMaximumHeight(maximumHeightColorWidget);

    auto hslLayout = new QGridLayout();
    hslLayout->addWidget(_hueAction.createLabelWidget(this), 0, 0);
    hslLayout->addWidget(_hueAction.createWidget(this), 0, 1);
    hslLayout->addWidget(_saturationAction.createLabelWidget(this), 1, 0);
    hslLayout->addWidget(_saturationAction.createWidget(this), 1, 1);
    hslLayout->addWidget(_lightnessAction.createLabelWidget(this), 2, 0);
    hslLayout->addWidget(_lightnessAction.createWidget(this), 2, 1);
    hslLayout->addWidget(_alphaAction.createLabelWidget(this), 3, 0);
    hslLayout->addWidget(_alphaAction.createWidget(this), 3, 1);

    // Set maximum height for hslLayout
    auto hslWidget = new QWidget();
    hslWidget->setLayout(hslLayout);
    hslWidget->setMaximumHeight(maximumHeightHSLWidget);

    // Create a new parent widget to hold both pickersWidget and hslWidget
    auto mainLayout = new QVBoxLayout();
    mainLayout->addWidget(pickersWidget);
    mainLayout->addWidget(hslWidget);

    auto mainWidget = new QWidget();
    mainWidget->setLayout(mainLayout);
    mainWidget->setMaximumHeight(maximumHeightColorWidget + maximumHeightHSLWidget + 10);

    _layout.addWidget(mainWidget);

    const auto updateColorFromHSL = [this, colorPickerAction]() -> void {
        if (!_updateColorPickerAction)
            return;

        colorPickerAction->setColor(QColor::fromHsl(_hueAction.getValue(), _saturationAction.getValue(), _lightnessAction.getValue(), _alphaAction.getValue()));
        };

    connect(&_hueAction, &IntegralAction::valueChanged, this, [this, updateColorFromHSL](const std::int32_t& value) {
        updateColorFromHSL();
        });

    connect(&_saturationAction, &IntegralAction::valueChanged, this, [this, updateColorFromHSL](const std::int32_t& value) {
        updateColorFromHSL();
        });

    connect(&_lightnessAction, &IntegralAction::valueChanged, this, [this, updateColorFromHSL](const std::int32_t& value) {
        updateColorFromHSL();
        });

    connect(&_alphaAction, &IntegralAction::valueChanged, this, [this, updateColorFromHSL](const std::int32_t& value) {
        updateColorFromHSL();
        });

    connect(colorPickerAction, &MaterialColorPickerAction::colorChanged, this, [this, colorPickerAction](const QColor& color) {
        _updateColorPickerAction = false;
        {
            _colorDialog.setCurrentColor(color);
            _hueAction.setValue(color.hue());
            _saturationAction.setValue(color.hslSaturation());
            _lightnessAction.setValue(color.lightness());
            _alphaAction.setValue(color.alpha());
        }
        _updateColorPickerAction = true;
        });

    connect(&_colorDialog, &QColorDialog::currentColorChanged, this, [this, colorPickerAction](const QColor& color) {
        if (!_updateColorPickerAction)
            return;

        colorPickerAction->setColor(color);
        });

    setLayout(&_layout);
}

void MaterialColorPickerAction::fromVariantMap(const QVariantMap& variantMap)
{
    WidgetAction::fromVariantMap(variantMap);

    //I removed to original safety check that was in the original ColorPickerAction as I couldn't get it to work

    setColor(variantMap["Value"].value<QColor>());
}

QVariantMap MaterialColorPickerAction::toVariantMap() const
{
    QVariantMap variantMap = WidgetAction::toVariantMap();

    variantMap.insert({
        { "Value", QVariant::fromValue(_color) }
        });

    return variantMap;
}
