#include "PositionAction.h"
#include "TransferFunctionPlugin.h"

#include <QMenu>

using namespace mv::gui;

PositionAction::PositionAction(QObject* parent, const QString& title) :
    VerticalGroupAction(parent, title),
    _xDimensionPickerAction(this, "X"),
    _yDimensionPickerAction(this, "Y"),
    _dontUpdateTransferFunction(false)
{
    setIcon(mv::Application::getIconFont("FontAwesome").getIcon("ruler-combined"));
    setLabelSizingType(LabelSizingType::Auto);

    addAction(&_xDimensionPickerAction);
    addAction(&_yDimensionPickerAction);

    _xDimensionPickerAction.setToolTip("X dimension");
    _yDimensionPickerAction.setToolTip("Y dimension");

    auto transferFunctionPlugin = dynamic_cast<TransferFunctionPlugin*>(parent->parent());

    if (transferFunctionPlugin == nullptr)
        return;

    connect(&_xDimensionPickerAction, &DimensionPickerAction::currentDimensionIndexChanged, [this, transferFunctionPlugin](const std::uint32_t& currentDimensionIndex) {
        if (_dontUpdateTransferFunction)
            return;

        transferFunctionPlugin->setXDimension(currentDimensionIndex);
    });

    connect(&_yDimensionPickerAction, &DimensionPickerAction::currentDimensionIndexChanged, [this, transferFunctionPlugin](const std::uint32_t& currentDimensionIndex) {
        if (_dontUpdateTransferFunction)
            return;

        transferFunctionPlugin->setYDimension(currentDimensionIndex);
    });

    const auto updateReadOnly = [this, transferFunctionPlugin]() -> void {
        setEnabled(transferFunctionPlugin->getPositionDataset().isValid());
        };

    updateReadOnly();

    auto onDimensionsChanged = [this, transferFunctionPlugin](int32_t xDim, int32_t yDim, auto& currentData) {
        // Ensure that we never access non-existing dimensions
        // Data is re-drawn for each setPointsDataset call
        // with the dimension index of the respective _xDimensionPickerAction set to 0
        // This prevents the _yDimensionPickerAction dimension being larger than 
        // the new numDimensions during the first setPointsDataset call
        _dontUpdateTransferFunction = true;
        _yDimensionPickerAction.setCurrentDimensionIndex(yDim);

        _xDimensionPickerAction.setPointsDataset(currentData);
        _yDimensionPickerAction.setPointsDataset(currentData);

        // setXDimension() ignores its argument and will update the 
        // transferFunction with both current dimension indices
        _xDimensionPickerAction.setCurrentDimensionIndex(xDim);
        _yDimensionPickerAction.setCurrentDimensionIndex(yDim);
        transferFunctionPlugin->setXDimension(xDim);
        _dontUpdateTransferFunction = false;
    };

    connect(&transferFunctionPlugin->getPositionDataset(), &Dataset<Points>::dataDimensionsChanged, this, [this, updateReadOnly, transferFunctionPlugin, onDimensionsChanged]() {
        updateReadOnly();
        
        // if the new number of dimensions allows it, keep the previous dimension indices
        auto xDim = _xDimensionPickerAction.getCurrentDimensionIndex();
        auto yDim = _yDimensionPickerAction.getCurrentDimensionIndex();
        
        const auto& currentData = transferFunctionPlugin->getPositionDataset();
        const auto numDimensions = static_cast<int32_t>(currentData->getNumDimensions());
        
        if (xDim >= numDimensions || yDim >= numDimensions)
        {
            xDim = 0;
            yDim = numDimensions >= 2 ? 1 : 0;
        }
        
        onDimensionsChanged(xDim, yDim, currentData);
    });

    connect(&transferFunctionPlugin->getPositionDataset(), &Dataset<Points>::changed, this, [this, transferFunctionPlugin, updateReadOnly, onDimensionsChanged]([[maybe_unused]] mv::DatasetImpl* dataset) {
        if (transferFunctionPlugin->getPositionDataset().isValid()) {
            updateReadOnly();

            const auto& currentData = transferFunctionPlugin->getPositionDataset();
            const auto numDimensions = static_cast<int32_t>(currentData->getNumDimensions());

            const int32_t xDim = 0;
            const int32_t yDim = numDimensions >= 2 ? 1 : 0;

            onDimensionsChanged(xDim, yDim, currentData);
        }
        else {
            _xDimensionPickerAction.setPointsDataset(Dataset<Points>());
            _yDimensionPickerAction.setPointsDataset(Dataset<Points>());
        }
    });

}

QMenu* PositionAction::getContextMenu(QWidget* parent /*= nullptr*/)
{
    auto menu = new QMenu("Position", parent);

    auto xDimensionMenu = new QMenu("X dimension");
    auto yDimensionMenu = new QMenu("Y dimension");

    xDimensionMenu->addAction(&_xDimensionPickerAction);
    yDimensionMenu->addAction(&_yDimensionPickerAction);

    menu->addMenu(xDimensionMenu);
    menu->addMenu(yDimensionMenu);

    return menu;
}

std::int32_t PositionAction::getDimensionX() const
{
    return _xDimensionPickerAction.getCurrentDimensionIndex();
}

std::int32_t PositionAction::getDimensionY() const
{
    return _yDimensionPickerAction.getCurrentDimensionIndex();
}

void PositionAction::connectToPublicAction(WidgetAction* publicAction, bool recursive)
{
    auto publicPositionAction = dynamic_cast<PositionAction*>(publicAction);

    Q_ASSERT(publicPositionAction != nullptr);

    if (publicPositionAction == nullptr)
        return;

    if (recursive) {
        actions().connectPrivateActionToPublicAction(&_xDimensionPickerAction, &publicPositionAction->getXDimensionPickerAction(), recursive);
        actions().connectPrivateActionToPublicAction(&_yDimensionPickerAction, &publicPositionAction->getYDimensionPickerAction(), recursive);
    }

    GroupAction::connectToPublicAction(publicAction, recursive);
}

void PositionAction::disconnectFromPublicAction(bool recursive)
{
    if (!isConnected())
        return;

    if (recursive) {
        actions().disconnectPrivateActionFromPublicAction(&_xDimensionPickerAction, recursive);
        actions().disconnectPrivateActionFromPublicAction(&_yDimensionPickerAction, recursive);
    }

    GroupAction::disconnectFromPublicAction(recursive);
}

void PositionAction::fromVariantMap(const QVariantMap& variantMap)
{
    GroupAction::fromVariantMap(variantMap);

    _xDimensionPickerAction.fromParentVariantMap(variantMap);
    _yDimensionPickerAction.fromParentVariantMap(variantMap);
}

QVariantMap PositionAction::toVariantMap() const
{
    auto variantMap = GroupAction::toVariantMap();

    _xDimensionPickerAction.insertIntoVariantMap(variantMap);
    _yDimensionPickerAction.insertIntoVariantMap(variantMap);

    return variantMap;
}
