#pragma once

#include <actions/ColorAction.h>
#include <actions/DatasetPickerAction.h>
#include <actions/GroupAction.h>
#include <actions/HorizontalGroupAction.h>
#include <actions/TriggerAction.h>
#include <actions/VerticalGroupAction.h>

using namespace mv;
using namespace mv::gui;
using namespace mv::util;

class Clusters;
class TransferFunctionPlugin;

/**
 * Clustering action class
 *
 * Action class for clustering
 *
 * @author Thomas Kroes
 */
class ClusteringAction final : public GroupAction
{
public:

    /**
     * Construct with \p parent object and \p title
     * @param parent Pointer to parent object
     * @param title Title
     */
    Q_INVOKABLE ClusteringAction(QObject* parent, const QString& title);

private:

    /** Come up with a pseudo-random cluster color and assign it to the color action */
    void randomizeClusterColor();

public: // Action getters

    StringAction& getNameAction() { return _nameAction; }
    ColorAction& getColorAction() { return _colorAction; }
    TriggerAction& getCreateCluster() { return _addClusterAction; }
    TriggerAction& getAddClusterAction() { return _addClusterAction; }
    DatasetPickerAction& getClusterDatasetPickerAction() { return _clusterDatasetPickerAction; }
    StringAction& getClusterDatasetNameAction() { return _clusterDatasetNameAction; }
    TriggerAction& getCreateClusterDatasetAction() { return _createClusterDatasetAction; }
    VerticalGroupAction& getClusterDatasetWizardAction() { return _clusterDatasetWizardAction; }
    HorizontalGroupAction& getTargetClusterDatasetAction() { return _clusterDatasetAction; }

private:
    TransferFunctionPlugin*      _transferFunctionPlugin;                     /** Pointer to scatter plot plugin */
    StringAction            _nameAction;                            /** Cluster name action */
    ColorAction             _colorAction;                           /** Cluster color action */
    TriggerAction           _addClusterAction;                      /** Add manual cluster action */
    DatasetPickerAction     _clusterDatasetPickerAction;      /** Dataset picker for picking the target cluster dataset */
    StringAction            _clusterDatasetNameAction;              /** Cluster dataset name action */
    TriggerAction           _createClusterDatasetAction;            /** Triggers the creation of a new cluster dataset */
    VerticalGroupAction     _clusterDatasetWizardAction;            /** Vertical group action for cluster wizard */
    HorizontalGroupAction   _clusterDatasetAction;                  /** Horizontal group action which groups the cluster dataset picker and the add cluster trigger action */
};
