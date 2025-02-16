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

gradientData MaterialColorPickerAction::getGradientData()
{
	return _gradientData;
}

QImage MaterialColorPickerAction::getGradientImage()
{
	return _gradientImage;
}

void MaterialColorPickerAction::setColor(const QColor& color)
{
    if (color == _color)
        return;

    _color = color;

    emit colorChanged(_color);
}

void MaterialColorPickerAction::setGradient(gradientData gradientData)
{
	_gradientData = gradientData;
	emit gradientChanged(_gradientData, _gradientImage);
}

void MaterialColorPickerAction::setGradient(gradientData gradientData, QImage gradientImage)
{
	_gradientImage = gradientImage;
    _gradientData = gradientData;
    emit gradientChanged(_gradientData, _gradientImage);
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
		setGradient(shape->getGradientData(), shape->getGradientImage());
        });


    connect(this, &MaterialColorPickerAction::colorChanged, &widget, [this, &widget](const QColor& color) {
        InteractiveShape* shape = widget.getSelectedObject();
        if (shape == nullptr)
            return;
        shape->setColor(color);
        widget.update();
        });

    connect(this, &MaterialColorPickerAction::gradientChanged, &widget, [this, &widget](const gradientData& gradientData) {
        InteractiveShape* shape = widget.getSelectedObject();
        if (shape == nullptr)
            return;
		shape->updateGradient(gradientData);
		//setGradient(gradientData, shape->getGradientImage()); // This is kind of a scuffed line as this whole class is mostly meant to sent information and only recieve it when a new wshape is selected
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

MaterialColorPickerAction::Widget::Widget(QWidget* parent, MaterialColorPickerAction* materialColorPickerAction) :
    WidgetActionWidget(parent, materialColorPickerAction),
    _layout(),
    _colorDialog(),
    _hueAction(this, "Hue", 0, 359, materialColorPickerAction->getColor().hue()),
    _saturationAction(this, "Saturation", 0, 255, materialColorPickerAction->getColor().hslSaturation()),
    _lightnessAction(this, "Lightness", 0, 255, materialColorPickerAction->getColor().lightness()),
    _alphaAction(this, "Alpha", 0, 255, materialColorPickerAction->getColor().alpha()),
    _updateElements(true),
	_gradientToggleAction(this, "Use Gradient", false),
	_gradientTextureIDAction(this, "Texture ID", 0, 1, 0),
	_gradientXOffsetAction(this, "X Offset", -1, 1, 0),
	_gradientYOffsetAction(this, "Y Offset", -1, 1, 0),
	_gradientWidthAction(this, "Width", 0.1, 5, 0),
	_gradientHeightAction(this, "Height", 0.1, 5, 0),
	_gradientRotationAction(this, "Rotation", 0, 90, 0),
	_gradientImageLabel(new QLabel(this))
{
    setAcceptDrops(true);

    _colorDialog.setCurrentColor(materialColorPickerAction->getColor());
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
    
    // color values layout
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

    auto fullColorWidget = new QWidget();
    fullColorWidget->setLayout(mainLayout);
    fullColorWidget->setMaximumHeight(maximumHeightColorWidget + maximumHeightHSLWidget + 10);

    _layout.addWidget(fullColorWidget);

	// Gradient layout
	auto gradientLayout = new QGridLayout();
	gradientLayout->addWidget(_gradientToggleAction.createLabelWidget(this), 0, 0);
	gradientLayout->addWidget(_gradientToggleAction.createWidget(this), 0, 1);
	gradientLayout->addWidget(_gradientTextureIDAction.createLabelWidget(this), 1, 0);
	gradientLayout->addWidget(_gradientTextureIDAction.createWidget(this), 1, 1);
	gradientLayout->addWidget(_gradientXOffsetAction.createLabelWidget(this), 2, 0);
	gradientLayout->addWidget(_gradientXOffsetAction.createWidget(this), 2, 1);
	gradientLayout->addWidget(_gradientYOffsetAction.createLabelWidget(this), 3, 0);
	gradientLayout->addWidget(_gradientYOffsetAction.createWidget(this), 3, 1);
	gradientLayout->addWidget(_gradientWidthAction.createLabelWidget(this), 4, 0);
	gradientLayout->addWidget(_gradientWidthAction.createWidget(this), 4, 1);
	gradientLayout->addWidget(_gradientHeightAction.createLabelWidget(this), 5, 0);
	gradientLayout->addWidget(_gradientHeightAction.createWidget(this), 5, 1);
	gradientLayout->addWidget(_gradientRotationAction.createLabelWidget(this), 6, 0);
	gradientLayout->addWidget(_gradientRotationAction.createWidget(this), 6, 1);

	auto gradientWidget = new QWidget();
	gradientWidget->setLayout(gradientLayout);
    gradientWidget->setMaximumHeight(250);

    // Create QLabel to display the gradient image
	_gradientImageLabel->setPixmap(QPixmap::fromImage(QImage(200, 200, QImage::Format_RGB32)));
    _gradientImageLabel->setAlignment(Qt::AlignCenter);
    _gradientImageLabel->setScaledContents(true); 
	_gradientImageLabel->setMaximumHeight(200);
    _gradientImageLabel->setMaximumWidth(200);

    // Create a new parent widget to hold both gradientImageLabel and gradientWidget
    auto gradientMainLayout = new QVBoxLayout();
    gradientMainLayout->addWidget(_gradientImageLabel);
    gradientMainLayout->addWidget(gradientWidget);

    auto fullGradientWidget = new QWidget();
    fullGradientWidget->setLayout(gradientMainLayout);
    fullGradientWidget->setMaximumHeight(450);

	_layout.addWidget(fullGradientWidget);

    // Color values connections
    const auto updateColorFromHSL = [this, materialColorPickerAction]() -> void {
        if (!_updateElements)
            return;

        materialColorPickerAction->setColor(QColor::fromHsl(_hueAction.getValue(), _saturationAction.getValue(), _lightnessAction.getValue(), _alphaAction.getValue()));
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

    connect(materialColorPickerAction, &MaterialColorPickerAction::colorChanged, this, [this, materialColorPickerAction](const QColor& color) {
        _updateElements = false;
        {
            _colorDialog.setCurrentColor(color);
            _hueAction.setValue(color.hue());
            _saturationAction.setValue(color.hslSaturation());
            _lightnessAction.setValue(color.lightness());
            _alphaAction.setValue(color.alpha());
        }
        _updateElements = true;
        });

    connect(&_colorDialog, &QColorDialog::currentColorChanged, this, [this, materialColorPickerAction](const QColor& color) {
        if (!_updateElements)
            return;

        materialColorPickerAction->setColor(color);
        });

	// Gradient value connections
	connect(&_gradientToggleAction, &ToggleAction::toggled, this, [this, materialColorPickerAction](const bool& value) {
		if (!_updateElements)
			return;
		gradientData data = materialColorPickerAction->getGradientData();
		data.gradient = value;
		materialColorPickerAction->setGradient(data);
		});

	connect(&_gradientTextureIDAction, &IntegralAction::valueChanged, this, [this, materialColorPickerAction](const std::int32_t& value) {
		if (!_updateElements)
			return;
		gradientData data = materialColorPickerAction->getGradientData();
		data.textureID = value;
		materialColorPickerAction->setGradient(data);
		});

	connect(&_gradientXOffsetAction, &DecimalAction::valueChanged, this, [this, materialColorPickerAction](const qreal& value) {
		if (!_updateElements)
			return;
		gradientData data = materialColorPickerAction->getGradientData();
		data.xOffset = value;
		materialColorPickerAction->setGradient(data);
		});

	connect(&_gradientYOffsetAction, &DecimalAction::valueChanged, this, [this, materialColorPickerAction](const qreal& value) {
		if (!_updateElements)
			return;
		gradientData data = materialColorPickerAction->getGradientData();
		data.yOffset = value;
		materialColorPickerAction->setGradient(data);
		});

	connect(&_gradientWidthAction, &DecimalAction::valueChanged, this, [this, materialColorPickerAction](const qreal& value) {
		if (!_updateElements)
			return;
		gradientData data = materialColorPickerAction->getGradientData();
		data.width = value;
		materialColorPickerAction->setGradient(data);
		});

	connect(&_gradientHeightAction, &DecimalAction::valueChanged, this, [this, materialColorPickerAction](const qreal& value) {
		if (!_updateElements)
			return;
		gradientData data = materialColorPickerAction->getGradientData();
		data.height = value;
		materialColorPickerAction->setGradient(data);
		});

    connect(&_gradientRotationAction, &IntegralAction::valueChanged, this, [this, materialColorPickerAction](const qreal& value) {
        if (!_updateElements)
            return;
        gradientData data = materialColorPickerAction->getGradientData();
        data.rotation = value;
        materialColorPickerAction->setGradient(data);
        });

	connect(materialColorPickerAction, &MaterialColorPickerAction::gradientChanged, this, [this, materialColorPickerAction](const gradientData& data, const QImage& image) {
		_updateElements = false;
		{
		    _gradientToggleAction.setChecked(data.gradient);
			_gradientTextureIDAction.setValue(data.textureID);
			_gradientXOffsetAction.setValue(data.xOffset);
			_gradientYOffsetAction.setValue(data.yOffset);
			_gradientWidthAction.setValue(data.width);
			_gradientHeightAction.setValue(data.height);
			_gradientRotationAction.setValue(data.rotation);
            _gradientImageLabel->setPixmap(QPixmap::fromImage(materialColorPickerAction->getGradientImage()));
        }
        _updateElements = true;
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
