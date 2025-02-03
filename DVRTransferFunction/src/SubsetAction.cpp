#include "SubsetAction.h"

#include "TransferFunctionPlugin.h"
#include "TransferFunctionWidget.h"

#include <PointData/PointData.h>

#include <Application.h>

#include <QMenu>

using namespace mv;
using namespace mv::gui;

SubsetAction::SubsetAction(QObject* parent, const QString& title) :
    VerticalGroupAction(parent, title),
    _transferFunctionPlugin(dynamic_cast<TransferFunctionPlugin*>(parent->parent())),
    _subsetNameAction(this, "Subset name"),
    _sourceDataAction(this, "Source data"),
    _createSubsetAction(this, "Create subset")
{
    setIcon(mv::Application::getIconFont("FontAwesome").getIcon("crop"));
    setConnectionPermissionsToForceNone(true);
    setConfigurationFlag(WidgetAction::ConfigurationFlag::ForceCollapsedInGroup);
    setLabelSizingType(LabelSizingType::Auto);

    addAction(&_subsetNameAction);
    addAction(&_sourceDataAction);
    addAction(&_createSubsetAction);

    _subsetNameAction.setToolTip("Name of the subset");
    _createSubsetAction.setToolTip("Create subset from selected data points");

    const auto updateReadOnly = [this]() -> void {
        _createSubsetAction.setEnabled(!_subsetNameAction.getString().isEmpty());
    };

    updateReadOnly();

    connect(&_subsetNameAction, &StringAction::stringChanged, this, updateReadOnly);
}

void SubsetAction::initialize(TransferFunctionPlugin* transferFunctionPlugin)
{
    Q_ASSERT(transferFunctionPlugin != nullptr);

    if (transferFunctionPlugin == nullptr)
        return;

    _transferFunctionPlugin = transferFunctionPlugin;

    connect(&_createSubsetAction, &QAction::triggered, this, [this]() {
        _transferFunctionPlugin->createSubset(_sourceDataAction.getCurrentIndex() == 1, _subsetNameAction.getString());
    });

    const auto onCurrentDatasetChanged = [this]() -> void {
        if (!_transferFunctionPlugin->getPositionDataset().isValid())
            return;

        const auto datasetGuiName = _transferFunctionPlugin->getPositionDataset()->text();

        QStringList sourceDataOptions;

        if (!datasetGuiName.isEmpty()) {
            const auto sourceDatasetGuiName = _transferFunctionPlugin->getPositionDataset()->getSourceDataset<Points>()->text();

            sourceDataOptions << QString("From: %1").arg(datasetGuiName);

            if (sourceDatasetGuiName != datasetGuiName)
                sourceDataOptions << QString("From: %1 (source data)").arg(sourceDatasetGuiName);
        }

        _sourceDataAction.setOptions(sourceDataOptions);
        _sourceDataAction.setEnabled(sourceDataOptions.count() >= 2);
    };

    connect(&_transferFunctionPlugin->getPositionDataset(), &Dataset<Points>::changed, this, onCurrentDatasetChanged);

    onCurrentDatasetChanged();

    const auto updateReadOnly = [this]() {
        const auto positionDataset          = _transferFunctionPlugin->getPositionDataset();
        const auto numberOfSelectedPoints   = positionDataset.isValid() ? positionDataset->getSelectionSize() : 0;
        const auto hasSelection             = numberOfSelectedPoints >= 1;

        setEnabled(_transferFunctionPlugin->getTransferFunctionWidget().getRenderMode() == TransferFunctionWidget::SCATTERPLOT && hasSelection);
    };

    updateReadOnly();

    connect(&_transferFunctionPlugin->getTransferFunctionWidget(), &TransferFunctionWidget::renderModeChanged, this, updateReadOnly);
    connect(&_transferFunctionPlugin->getPositionDataset(), &Dataset<Points>::dataSelectionChanged, this, updateReadOnly);
}

QMenu* SubsetAction::getContextMenu()
{
    auto menu = new QMenu("Subset");

    menu->addAction(&_createSubsetAction);
    menu->addAction(&_sourceDataAction);

    return menu;
}

void SubsetAction::fromVariantMap(const QVariantMap& variantMap)
{
    GroupAction::fromVariantMap(variantMap);

    _subsetNameAction.fromParentVariantMap(variantMap);
}

QVariantMap SubsetAction::toVariantMap() const
{
    auto variantMap = GroupAction::toVariantMap();

    _subsetNameAction.insertIntoVariantMap(variantMap);

    return variantMap;
}
