#include "ColoringAction.h"
#include "TransferFunctionPlugin.h"
#include "TransferFunctionWidget.h"
#include "DataHierarchyItem.h"

#include <PointData/PointData.h>
#include <ClusterData/ClusterData.h>

using namespace mv::gui;

const QColor ColoringAction::DEFAULT_CONSTANT_COLOR = qRgb(93, 93, 225);

ColoringAction::ColoringAction(QObject* parent, const QString& title) :
    VerticalGroupAction(parent, title),
    _transferFunctionPlugin(dynamic_cast<TransferFunctionPlugin*>(parent->parent())),
    _colorByModel(this),
    _colorByAction(this, "Color by"),
    _constantColorAction(this, "Constant color", DEFAULT_CONSTANT_COLOR),
    _dimensionAction(this, "Dimension"),
    _colorMap1DAction(this, "1D Color map"),
    _colorMap2DAction(this, "2D Color map")
{
    setIcon(mv::Application::getIconFont("FontAwesome").getIcon("palette"));
    setLabelSizingType(LabelSizingType::Auto);
    setConfigurationFlag(WidgetAction::ConfigurationFlag::ForceCollapsedInGroup);

    addAction(&_colorByAction);
    addAction(&_constantColorAction);
    addAction(&_colorMap2DAction);
    addAction(&_colorMap1DAction);
    addAction(&_dimensionAction);

    _transferFunctionPlugin->getWidget().addAction(&_colorByAction);
    _transferFunctionPlugin->getWidget().addAction(&_dimensionAction);

    _colorByAction.setCustomModel(&_colorByModel);
    _colorByAction.setToolTip("Color by");

    connect(&_transferFunctionPlugin->getPositionDataset(), &Dataset<Points>::changed, this, [this]() {
        const auto positionDataset = _transferFunctionPlugin->getPositionDataset();

        if (!positionDataset.isValid())
            return;

        _colorByModel.removeAllDatasets();

        addColorDataset(positionDataset);

        const auto positionSourceDataset = _transferFunctionPlugin->getPositionSourceDataset();

        if (positionSourceDataset.isValid())
            addColorDataset(positionSourceDataset);

        updateColorByActionOptions();

        _colorByAction.setCurrentIndex(0);
    });

    connect(&_transferFunctionPlugin->getPositionSourceDataset(), &Dataset<Points>::changed, this, [this]() {
        const auto positionSourceDataset = _transferFunctionPlugin->getPositionSourceDataset();

        if (positionSourceDataset.isValid())
            addColorDataset(positionSourceDataset);

        updateColorByActionOptions();
    });

    connect(&_colorByAction, &OptionAction::currentIndexChanged, this, [this](const std::int32_t& currentIndex) {
        _transferFunctionPlugin->getTransferFunctionWidget().setColoringMode(currentIndex == 0 ? TransferFunctionWidget::ColoringMode::Constant : TransferFunctionWidget::ColoringMode::Data);

        _constantColorAction.setEnabled(currentIndex == 0);

        const auto currentColorDataset = getCurrentColorDataset();

        if (currentColorDataset.isValid()) {
            const auto currentColorDatasetTypeIsPointType = currentColorDataset->getDataType() == PointType;

            _dimensionAction.setPointsDataset(currentColorDatasetTypeIsPointType ? Dataset<Points>(currentColorDataset) : Dataset<Points>());
            //_dimensionAction.setVisible(currentColorDatasetTypeIsPointType);

            emit currentColorDatasetChanged(currentColorDataset);
        }
        else {
            _dimensionAction.setPointsDataset(Dataset<Points>());
            //_dimensionAction.setVisible(false);
        }

        updateTransferFunctionWidgetColors();
        updateTransferFunctionWidgetColorMap();
        updateColorMapActionScalarRange();
        updateColorMapActionsReadOnly();
    });

    connect(&_colorByModel, &ColorSourceModel::dataChanged, this, [this](const Dataset<DatasetImpl>& dataset) {
        const auto currentColorDataset = getCurrentColorDataset();

        if (!(currentColorDataset.isValid() && dataset.isValid()))
            return;

        if (currentColorDataset == dataset)
            updateTransferFunctionWidgetColors();
        });

    connect(&_transferFunctionPlugin->getPositionDataset(), &Dataset<Points>::childAdded, this, &ColoringAction::updateColorByActionOptions);
    connect(&_transferFunctionPlugin->getPositionDataset(), &Dataset<Points>::childRemoved, this, &ColoringAction::updateColorByActionOptions);

    connect(&_transferFunctionPlugin->getTransferFunctionWidget(), &TransferFunctionWidget::renderModeChanged, this, &ColoringAction::updateTransferFunctionWidgetColors);
    connect(&_transferFunctionPlugin->getTransferFunctionWidget(), &TransferFunctionWidget::coloringModeChanged, this, &ColoringAction::updateTransferFunctionWidgetColors);

    connect(&_dimensionAction, &DimensionPickerAction::currentDimensionIndexChanged, this, &ColoringAction::updateTransferFunctionWidgetColors);
    connect(&_dimensionAction, &DimensionPickerAction::currentDimensionIndexChanged, this, &ColoringAction::updateColorMapActionScalarRange);
    
    connect(&_constantColorAction, &ColorAction::colorChanged, this, &ColoringAction::updateTransferFunctionWidgetColorMap);
    connect(&_colorMap1DAction, &ColorMapAction::imageChanged, this, &ColoringAction::updateTransferFunctionWidgetColorMap);
    connect(&_colorMap2DAction, &ColorMapAction::imageChanged, this, &ColoringAction::updateTransferFunctionWidgetColorMap);
    connect(&_transferFunctionPlugin->getTransferFunctionWidget(), &TransferFunctionWidget::coloringModeChanged, this, &ColoringAction::updateTransferFunctionWidgetColorMap);
    connect(&_transferFunctionPlugin->getTransferFunctionWidget(), &TransferFunctionWidget::renderModeChanged, this, &ColoringAction::updateTransferFunctionWidgetColorMap);

    connect(&_colorMap1DAction.getRangeAction(ColorMapAction::Axis::X), &DecimalRangeAction::rangeChanged, this, &ColoringAction::updateTransferFunctionWidgetColorMapRange);
    connect(&_colorMap2DAction.getRangeAction(ColorMapAction::Axis::X), &DecimalRangeAction::rangeChanged, this, &ColoringAction::updateTransferFunctionWidgetColorMapRange);

    connect(&_transferFunctionPlugin->getTransferFunctionWidget(), &TransferFunctionWidget::coloringModeChanged, this, &ColoringAction::updateColorMapActionsReadOnly);
    connect(&_transferFunctionPlugin->getTransferFunctionWidget(), &TransferFunctionWidget::renderModeChanged, this, &ColoringAction::updateColorMapActionsReadOnly);

    const auto updateReadOnly = [this]() {
        setEnabled(_transferFunctionPlugin->getPositionDataset().isValid() && _transferFunctionPlugin->getTransferFunctionWidget().getRenderMode() == TransferFunctionWidget::SCATTERPLOT);
    };

    updateReadOnly();

    connect(&_transferFunctionPlugin->getTransferFunctionWidget(), &TransferFunctionWidget::renderModeChanged, this, updateReadOnly);

    updateTransferFunctionWidgetColorMap();
    updateColorMapActionScalarRange();

    _transferFunctionPlugin->getTransferFunctionWidget().setColoringMode(TransferFunctionWidget::ColoringMode::Constant);
}

QMenu* ColoringAction::getContextMenu(QWidget* parent /*= nullptr*/)
{
    auto menu = new QMenu("Color", parent);

    const auto addActionToMenu = [menu](QAction* action) -> void {
        auto actionMenu = new QMenu(action->text());

        actionMenu->addAction(action);

        menu->addMenu(actionMenu);
    };

    return menu;
}

void ColoringAction::addColorDataset(const Dataset<DatasetImpl>& colorDataset)
{
    if (!colorDataset.isValid())
        return;

    if (hasColorDataset(colorDataset))
        return;

    _colorByModel.addDataset(colorDataset);
}

bool ColoringAction::hasColorDataset(const Dataset<DatasetImpl>& colorDataset) const
{
    return _colorByModel.rowIndex(colorDataset) >= 0;
}

Dataset<DatasetImpl> ColoringAction::getCurrentColorDataset() const
{
    const auto colorByIndex = _colorByAction.getCurrentIndex();

    if (colorByIndex < 2)
        return Dataset<DatasetImpl>();

    return _colorByModel.getDataset(colorByIndex);
}

void ColoringAction::setCurrentColorDataset(const Dataset<DatasetImpl>& colorDataset)
{
    addColorDataset(colorDataset);

    const auto colorDatasetRowIndex = _colorByModel.rowIndex(colorDataset);

    if (colorDatasetRowIndex >= 0)
        _colorByAction.setCurrentIndex(colorDatasetRowIndex);

    emit currentColorDatasetChanged(colorDataset);
}

void ColoringAction::updateColorByActionOptions()
{
    auto positionDataset = _transferFunctionPlugin->getPositionDataset();

    if (!positionDataset.isValid())
        return;

    const auto children = positionDataset->getDataHierarchyItem().getChildren();

    for (auto child : children) {
        const auto childDataset = child->getDataset();
        const auto dataType     = childDataset->getDataType();

        if (dataType == PointType || dataType == ClusterType)
            addColorDataset(childDataset);
    }
}

void ColoringAction::updateTransferFunctionWidgetColors()
{
    if (_colorByAction.getCurrentIndex() <= 1)
        return;

    const auto currentColorDataset = getCurrentColorDataset();

    if (!currentColorDataset.isValid())
        return;

    const auto currentDimensionIndex = _dimensionAction.getCurrentDimensionIndex();

    if (currentDimensionIndex >= 0)
        _transferFunctionPlugin->loadColors(currentColorDataset.get<Points>(), _dimensionAction.getCurrentDimensionIndex());

    updateTransferFunctionWidgetColorMap();
}

void ColoringAction::updateColorMapActionScalarRange()
{
    const auto colorMapRange    = _transferFunctionPlugin->getTransferFunctionWidget().getColorMapRange();
    const auto colorMapRangeMin = colorMapRange.x;
    const auto colorMapRangeMax = colorMapRange.y;

    auto& colorMapRangeAction = _colorMap1DAction.getRangeAction(ColorMapAction::Axis::X);

    colorMapRangeAction.initialize({ colorMapRangeMin, colorMapRangeMax }, { colorMapRangeMin, colorMapRangeMax });
	
	_colorMap1DAction.getDataRangeAction(ColorMapAction::Axis::X).setRange({ colorMapRangeMin, colorMapRangeMax });
}

void ColoringAction::updateTransferFunctionWidgetColorMap()
{
    auto& transferFunctionWidget = _transferFunctionPlugin->getTransferFunctionWidget();

    switch (transferFunctionWidget.getRenderMode())
    {
        case TransferFunctionWidget::SCATTERPLOT:
        {
            if (_colorByAction.getCurrentIndex() == 0) {
                QPixmap colorPixmap(1, 1);

                colorPixmap.fill(_constantColorAction.getColor());

                transferFunctionWidget.setColorMap(colorPixmap.toImage());
                transferFunctionWidget.setScalarEffect(PointEffect::Color);
                transferFunctionWidget.setColoringMode(TransferFunctionWidget::ColoringMode::Constant);
            }
            else if (_colorByAction.getCurrentIndex() == 1) {
                transferFunctionWidget.setColorMap(_colorMap2DAction.getColorMapImage());
                transferFunctionWidget.setScalarEffect(PointEffect::Color2D);
                transferFunctionWidget.setColoringMode(TransferFunctionWidget::ColoringMode::Scatter);
            }
            else {
                transferFunctionWidget.setColorMap(_colorMap1DAction.getColorMapImage().mirrored(false, true));
            }

            break;
        }

        case TransferFunctionWidget::DENSITY:
            break;

        case TransferFunctionWidget::LANDSCAPE:
        {
            transferFunctionWidget.setScalarEffect(PointEffect::Color);
            transferFunctionWidget.setColoringMode(TransferFunctionWidget::ColoringMode::Scatter);
            transferFunctionWidget.setColorMap(_colorMap1DAction.getColorMapImage());
            
            break;
        }

        default:
            break;
    }

    updateTransferFunctionWidgetColorMapRange();
}

void ColoringAction::updateTransferFunctionWidgetColorMapRange()
{
    const auto& rangeAction = _colorMap1DAction.getRangeAction(ColorMapAction::Axis::X);

    _transferFunctionPlugin->getTransferFunctionWidget().setColorMapRange(rangeAction.getMinimum(), rangeAction.getMaximum());
}

bool ColoringAction::shouldEnableColorMap() const
{
    if (!_transferFunctionPlugin->getPositionDataset().isValid())
        return false;

    const auto currentColorDataset = getCurrentColorDataset();

    if (currentColorDataset.isValid() && currentColorDataset->getDataType() == ClusterType)
        return false;

    if (_transferFunctionPlugin->getTransferFunctionWidget().getRenderMode() == TransferFunctionWidget::LANDSCAPE)
        return true;

    if (_transferFunctionPlugin->getTransferFunctionWidget().getRenderMode() == TransferFunctionWidget::SCATTERPLOT && _colorByAction.getCurrentIndex() > 0)
        return true;

    return false;
}

void ColoringAction::updateColorMapActionsReadOnly()
{
    const auto currentIndex = _colorByAction.getCurrentIndex();

    _colorMap1DAction.setEnabled(shouldEnableColorMap() && (currentIndex >= 2));
    _colorMap2DAction.setEnabled(shouldEnableColorMap() && (currentIndex == 1));
}

void ColoringAction::connectToPublicAction(WidgetAction* publicAction, bool recursive)
{
    auto publicColoringAction = dynamic_cast<ColoringAction*>(publicAction);

    Q_ASSERT(publicColoringAction != nullptr);

    if (publicColoringAction == nullptr)
        return;

    if (recursive) {
        actions().connectPrivateActionToPublicAction(&_colorByAction, &publicColoringAction->getColorByAction(), recursive);
        actions().connectPrivateActionToPublicAction(&_constantColorAction, &publicColoringAction->getConstantColorAction(), recursive);
        actions().connectPrivateActionToPublicAction(&_dimensionAction, &publicColoringAction->getDimensionAction(), recursive);
        actions().connectPrivateActionToPublicAction(&_colorMap1DAction, &publicColoringAction->getColorMap1DAction(), recursive);
        actions().connectPrivateActionToPublicAction(&_colorMap2DAction, &publicColoringAction->getColorMap2DAction(), recursive);
    }

    GroupAction::connectToPublicAction(publicAction, recursive);
}

void ColoringAction::disconnectFromPublicAction(bool recursive)
{
    if (!isConnected())
        return;

    if (recursive) {
        actions().disconnectPrivateActionFromPublicAction(&_colorByAction, recursive);
        actions().disconnectPrivateActionFromPublicAction(&_constantColorAction, recursive);
        actions().disconnectPrivateActionFromPublicAction(&_dimensionAction, recursive);
        actions().disconnectPrivateActionFromPublicAction(&_colorMap2DAction, recursive);
    }

    GroupAction::disconnectFromPublicAction(recursive);
}

void ColoringAction::fromVariantMap(const QVariantMap& variantMap)
{
    GroupAction::fromVariantMap(variantMap);

    _colorByAction.fromParentVariantMap(variantMap);
    _constantColorAction.fromParentVariantMap(variantMap);
    _dimensionAction.fromParentVariantMap(variantMap);
    _colorMap1DAction.fromParentVariantMap(variantMap);
    _colorMap2DAction.fromParentVariantMap(variantMap);
}

QVariantMap ColoringAction::toVariantMap() const
{
    auto variantMap = GroupAction::toVariantMap();

    _colorByAction.insertIntoVariantMap(variantMap);
    _constantColorAction.insertIntoVariantMap(variantMap);
    _dimensionAction.insertIntoVariantMap(variantMap);
    _colorMap1DAction.insertIntoVariantMap(variantMap);
    _colorMap2DAction.insertIntoVariantMap(variantMap);

    return variantMap;
}
