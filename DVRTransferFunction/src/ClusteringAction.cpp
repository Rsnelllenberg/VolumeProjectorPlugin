#include "ClusteringAction.h"
#include "TransferFunctionPlugin.h"

#include <ClusterData/ClusterData.h>
#include <PointData/PointData.h>

#include <QHBoxLayout>

using namespace mv;
using namespace mv::gui;

ClusteringAction::ClusteringAction(QObject* parent, const QString& title) :
    GroupAction(parent, title),
    _transferFunctionPlugin(dynamic_cast<TransferFunctionPlugin*>(parent->parent())),
    _nameAction(this, "Cluster name"),
    _colorAction(this, "Cluster color"),
    _addClusterAction(this, "Add cluster"),
    _clusterDatasetPickerAction(this, "Add to"),
    _clusterDatasetNameAction(this, "Dataset name"),
    _createClusterDatasetAction(this, "Create"),
    _clusterDatasetWizardAction(this, "Create cluster dataset"),
    _clusterDatasetAction(this, "Target clusters dataset")
{

}

void ClusteringAction::randomizeClusterColor()
{
    auto rng = QRandomGenerator::global();

    _colorAction.setColor(QColor::fromHsl(rng->bounded(360), rng->bounded(150, 255), rng->bounded(100, 200)));
}
