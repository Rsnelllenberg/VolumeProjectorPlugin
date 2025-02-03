#pragma once

#include <actions/GroupAction.h>

#include "DatasetsAction.h"
#include "SelectionAction.h"
#include "PointAction.h"
#include <PointData/DimensionPickerAction.h>

using namespace mv::gui;

class TransferFunctionPlugin;

/**
 * Settings action class
 *
 * Action class for configuring settings
 *
 * @author Thomas Kroes
 */
class SettingsAction : public GroupAction
{
public:
    
    /**
     * Construct with \p parent object and \p title
     * @param parent Pointer to parent object
     * @param title Title
     */
    Q_INVOKABLE SettingsAction(QObject* parent, const QString& title);

    /**
     * Get action context menu
     * @return Pointer to menu
     */
    QMenu* getContextMenu();

public: // Serialization

    /**
     * Load plugin from variant map
     * @param variantMap Variant map representation of the plugin
     */
    void fromVariantMap(const QVariantMap& variantMap) override;

    /**
     * Save plugin to variant map
     * @return Variant map representation of the plugin
     */
    QVariantMap toVariantMap() const override;

public: // Action getters
    
    SelectionAction& getSelectionAction() { return _selectionAction; }
    DatasetsAction& getDatasetsAction() { return _datasetsAction; }
	PointAction& getPointsAction() { return _pointsAction; }

protected:
    TransferFunctionPlugin* _transferFunctionPlugin;         /** Pointer to scatter plot plugin */

    SelectionAction             _selectionAction;           /** Action for selecting points */
    DatasetsAction              _datasetsAction;            /** Action for picking dataset(s) */
    PointAction				    _pointsAction; 			    /** Action for picking points appearance */
};