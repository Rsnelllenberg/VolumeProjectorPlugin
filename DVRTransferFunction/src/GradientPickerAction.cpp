#include "GradientPickerAction.h"
#include "TransferFunctionWidget.h"
#include "TransferFunctionPlugin.h"
#include "Application.h"

#include <QDebug>
#include <QHBoxLayout>
#include "InteractiveShape.h"

using namespace mv::gui;

GradientPickerAction::GradientPickerAction(QObject* parent, const QString& title) :
    WidgetAction(parent, title)
{
    setText(title);
}

gradientData GradientPickerAction::getGradientData() const
{
    return _gradientData;
}

QImage GradientPickerAction::getGradientImage() const
{
    return _gradientImage;
}

void GradientPickerAction::setGradient(const gradientData& gradientData)
{
    _gradientData = gradientData;
    emit gradientChanged(_gradientData, _gradientImage);
}

void GradientPickerAction::setGradientAndImage(const gradientData& gradientData, const QImage& gradientImage)
{
    _gradientImage = gradientImage;
    _gradientData = gradientData;
    emit gradientChanged(_gradientData, _gradientImage);
}

void GradientPickerAction::initialize(TransferFunctionPlugin* transferFunctionPlugin)
{
    Q_ASSERT(transferFunctionPlugin != nullptr);

    if (transferFunctionPlugin == nullptr)
        return;

    TransferFunctionWidget& widget = transferFunctionPlugin->getTransferFunctionWidget();

    connect(&widget, &TransferFunctionWidget::shapeSelected, this, [this](InteractiveShape* shape) {
        if (shape == nullptr)
            return;
        setGradientAndImage(shape->getGradientData(), shape->getGradientImage());
    });

    connect(this, &GradientPickerAction::gradientChanged, &widget, [this, &widget](const gradientData& gradientData) {
        InteractiveShape* shape = widget.getSelectedObject();
        if (shape == nullptr)
            return;
        shape->updateGradient(gradientData);
        //setGradientAndImage(gradientData, shape->getGradientImage()); // This is kind of a scuffed line as this whole class is mostly meant to sent information and only recieve it when a new wshape is selected
        widget.update();
        });
}

void GradientPickerAction::connectToPublicAction(WidgetAction* publicAction, bool recursive)
{
    auto publicGradientPickerAction = dynamic_cast<GradientPickerAction*>(publicAction);

    Q_ASSERT(publicGradientPickerAction != nullptr);

    if (publicGradientPickerAction == nullptr)
        return;

    connect(this, &GradientPickerAction::gradientChanged, publicGradientPickerAction, &GradientPickerAction::setGradient);
    connect(publicGradientPickerAction, &GradientPickerAction::gradientChanged, this, &GradientPickerAction::setGradient);

    setGradient(publicGradientPickerAction->getGradientData());

    WidgetAction::connectToPublicAction(publicAction, recursive);
}

void GradientPickerAction::disconnectFromPublicAction(bool recursive)
{
    if (!isConnected())
        return;

    auto publicGradientPickerAction = dynamic_cast<GradientPickerAction*>(getPublicAction());

    Q_ASSERT(publicGradientPickerAction != nullptr);

    if (publicGradientPickerAction == nullptr)
        return;

    disconnect(this, &GradientPickerAction::gradientChanged, publicGradientPickerAction, &GradientPickerAction::setGradient);
    disconnect(publicGradientPickerAction, &GradientPickerAction::gradientChanged, this, &GradientPickerAction::setGradient);

    WidgetAction::disconnectFromPublicAction(recursive);
}

GradientPickerAction::Widget::Widget(QWidget* parent, GradientPickerAction* gradientPickerAction) :
    WidgetActionWidget(parent, gradientPickerAction),
    _layout(),
    _gradientToggleAction(this, "Use Gradient", false),
    _gradientTextureIDAction(this, "Texture ID", 0, 1, 0),
    _gradientXOffsetAction(this, "X Offset", -1, 1, 0),
    _gradientYOffsetAction(this, "Y Offset", -1, 1, 0),
    _gradientWidthAction(this, "Width", 0.1, 5, 0),
    _gradientHeightAction(this, "Height", 0.1, 5, 0),
    _gradientRotationAction(this, "Rotation", 0, 90, 0),
    _gradientImageLabel(new QLabel(this)),
    _updateElements(true)
{
    setAcceptDrops(true);

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

    // Gradient value connections
    connect(&_gradientToggleAction, &ToggleAction::toggled, this, [this, gradientPickerAction](const bool& value) {
        if (!_updateElements)
            return;
        gradientData data = gradientPickerAction->getGradientData();
        data.gradient = value;
        gradientPickerAction->setGradient(data);
    });

    connect(&_gradientTextureIDAction, &IntegralAction::valueChanged, this, [this, gradientPickerAction](const std::int32_t& value) {
        if (!_updateElements)
            return;
        gradientData data = gradientPickerAction->getGradientData();
        data.textureID = value;
        gradientPickerAction->setGradient(data);
    });

    connect(&_gradientXOffsetAction, &DecimalAction::valueChanged, this, [this, gradientPickerAction](const qreal& value) {
        if (!_updateElements)
            return;
        gradientData data = gradientPickerAction->getGradientData();
        data.xOffset = value;
        gradientPickerAction->setGradient(data);
    });

    connect(&_gradientYOffsetAction, &DecimalAction::valueChanged, this, [this, gradientPickerAction](const qreal& value) {
        if (!_updateElements)
            return;
        gradientData data = gradientPickerAction->getGradientData();
        data.yOffset = value;
        gradientPickerAction->setGradient(data);
    });

    connect(&_gradientWidthAction, &DecimalAction::valueChanged, this, [this, gradientPickerAction](const qreal& value) {
        if (!_updateElements)
            return;
        gradientData data = gradientPickerAction->getGradientData();
        data.width = value;
        gradientPickerAction->setGradient(data);
    });

    connect(&_gradientHeightAction, &DecimalAction::valueChanged, this, [this, gradientPickerAction](const qreal& value) {
        if (!_updateElements)
            return;
        gradientData data = gradientPickerAction->getGradientData();
        data.height = value;
        gradientPickerAction->setGradient(data);
    });

    connect(&_gradientRotationAction, &IntegralAction::valueChanged, this, [this, gradientPickerAction](const qreal& value) {
        if (!_updateElements)
            return;
        gradientData data = gradientPickerAction->getGradientData();
        data.rotation = value;
        gradientPickerAction->setGradient(data);
    });

    connect(gradientPickerAction, &GradientPickerAction::gradientChanged, this, [this, gradientPickerAction](const gradientData& data, const QImage& image) {
        _updateElements = false;
        {
            _gradientToggleAction.setChecked(data.gradient);
            _gradientTextureIDAction.setValue(data.textureID);
            _gradientXOffsetAction.setValue(data.xOffset);
            _gradientYOffsetAction.setValue(data.yOffset);
            _gradientWidthAction.setValue(data.width);
            _gradientHeightAction.setValue(data.height);
            _gradientRotationAction.setValue(data.rotation);
            _gradientImageLabel->setPixmap(QPixmap::fromImage(gradientPickerAction->getGradientImage()));
        }
        _updateElements = true;
    });

    setLayout(&_layout);
}
