#pragma once

#include <actions/GroupAction.h>

#include "MaterialColorPickerAction.h" 
#include "GradientPickerAction.h"
//#include "TransferFunctionPlugin.h"

using namespace mv::gui;


class TransferFunctionPlugin;

/**
 * Material settings action class
 *
 * Action class for configuring material settings
 */
class MaterialSettings : public GroupAction
{
public:

    /**
     * Construct with \p parent object and \p title
     * @param parent Pointer to parent object
     * @param title Title
     */
    Q_INVOKABLE MaterialSettings(QObject* parent, const QString& title);

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

    MaterialColorPickerAction& getColorPickerAction() { return _colorPickerAction; }

protected:
    TransferFunctionPlugin* _transferFunctionPlugin;    /** Pointer to scatter plot plugin */
    MaterialColorPickerAction   _colorPickerAction;         /** Action for picking color */
	GradientPickerAction        _gradientPickerAction;      /** Action for picking gradient */
};
