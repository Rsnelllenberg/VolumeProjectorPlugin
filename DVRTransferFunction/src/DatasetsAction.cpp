#include "DatasetsAction.h"
#include "TransferFunctionPlugin.h"

#include <PointData/PointData.h>
#include <ClusterData/ClusterData.h>
#include <ColorData/ColorData.h>

#include <QMenu>

using namespace mv;
using namespace mv::gui;

DatasetsAction::DatasetsAction(QObject* parent, const QString& title) :
    GroupAction(parent, title),
    _transferFunctionPlugin(dynamic_cast<TransferFunctionPlugin*>(parent->parent())),
    _positionDatasetPickerAction(this, "Position"),
    _colorDatasetPickerAction(this, "Color")
{
    setIcon(mv::Application::getIconFont("FontAwesome").getIcon("database"));
    setToolTip("Manage loaded datasets for position and color");
    setConfigurationFlag(WidgetAction::ConfigurationFlag::ForceCollapsedInGroup);
    setLabelSizingType(LabelSizingType::Auto);

    addAction(&_positionDatasetPickerAction);
    addAction(&_colorDatasetPickerAction);

    _positionDatasetPickerAction.setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
        return dataset->getDataType() == PointType;
        });

    _colorDatasetPickerAction.setFilterFunction([this](mv::Dataset<DatasetImpl> dataset) -> bool {
        return (dataset->getDataType() == PointType || dataset->getDataType() == ColorType || dataset->getDataType() == ClusterType);
        });


    auto transferFunctionPlugin = dynamic_cast<TransferFunctionPlugin*>(parent->parent());

    if (transferFunctionPlugin == nullptr)
        return;

    connect(&_positionDatasetPickerAction, &DatasetPickerAction::datasetPicked, [this, transferFunctionPlugin](Dataset<DatasetImpl> pickedDataset) -> void {
        transferFunctionPlugin->getPositionDataset() = pickedDataset;
    });

    connect(&transferFunctionPlugin->getPositionDataset(), &Dataset<Points>::changed, this, [this](DatasetImpl* dataset) -> void {
        _positionDatasetPickerAction.setCurrentDataset(dataset);
    });

}

void DatasetsAction::connectToPublicAction(WidgetAction* publicAction, bool recursive)
{
    auto publicDatasetsAction = dynamic_cast<DatasetsAction*>(publicAction);

    Q_ASSERT(publicDatasetsAction != nullptr);

    if (publicDatasetsAction == nullptr)
        return;

    if (recursive) {
        actions().connectPrivateActionToPublicAction(&_positionDatasetPickerAction, &publicDatasetsAction->getPositionDatasetPickerAction(), recursive);
        actions().connectPrivateActionToPublicAction(&_colorDatasetPickerAction, &publicDatasetsAction->getColorDatasetPickerAction(), recursive);
    }

    GroupAction::connectToPublicAction(publicAction, recursive);
}

void DatasetsAction::disconnectFromPublicAction(bool recursive)
{
    if (!isConnected())
        return;

    if (recursive) {
        actions().disconnectPrivateActionFromPublicAction(&_positionDatasetPickerAction, recursive);
        actions().disconnectPrivateActionFromPublicAction(&_colorDatasetPickerAction, recursive);
    }

    GroupAction::disconnectFromPublicAction(recursive);
}

void DatasetsAction::fromVariantMap(const QVariantMap& variantMap)
{
    GroupAction::fromVariantMap(variantMap);

    _positionDatasetPickerAction.fromParentVariantMap(variantMap);
    _colorDatasetPickerAction.fromParentVariantMap(variantMap);
}

QVariantMap DatasetsAction::toVariantMap() const
{
    auto variantMap = GroupAction::toVariantMap();

    _positionDatasetPickerAction.insertIntoVariantMap(variantMap);
    _colorDatasetPickerAction.insertIntoVariantMap(variantMap);

    return variantMap;
}