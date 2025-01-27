#pragma once

#include <PluginGlobalSettingsGroupAction.h>
#include "GlobalSettingsAction.h"
#include <actions/DecimalRangeAction.h>
#include <actions/OptionAction.h>
#include <pointdata/DimensionPickerAction.h>

namespace mv {
    namespace plugin {
        class PluginFactory;
    }
}

/**
 * Global settings action class
 *
 * Action class for configuring global settings
 * 
 * This group action (once assigned to the plugin factory, see DVRViewPluginFactory::initialize()) is 
 * added to the global settings panel, accessible through the file > settings menu.
 * 
 */
class GlobalSettingsAction : public mv::gui::PluginGlobalSettingsGroupAction
{
public:

    /**
     * Construct with pointer to \p parent object and \p pluginFactory
     * @param parent Pointer to parent object
     * @param pluginFactory Pointer to plugin factory
     */
    Q_INVOKABLE GlobalSettingsAction(QObject* parent, const mv::plugin::PluginFactory* pluginFactory);

public: // Action getters

    mv::gui::DecimalRangeAction& getDefaultxDimClippingPlaneAction() { return _defaultXDimClippingPlaneAction; }
    mv::gui::DecimalRangeAction& getDefaultyDimClippingPlaneAction() { return _defaultYDimClippingPlaneAction; }
    mv::gui::DecimalRangeAction& getDefaultzDimClippingPlaneAction() { return _defaultZDimClippingPlaneAction; }

    mv::gui::OptionAction& getDefaultRenderModeAction() { return _defaultRenderModeAction; }
    DimensionPickerAction& getDefaultMIPDimensionAction() { return _defaultMIPDimensionAction; }

private:
    mv::gui::DecimalRangeAction     _defaultXDimClippingPlaneAction;       /** Default range size action */
    mv::gui::DecimalRangeAction     _defaultYDimClippingPlaneAction;       /** Default range size action */
    mv::gui::DecimalRangeAction     _defaultZDimClippingPlaneAction;       /** Default range size action */

    mv::gui::OptionAction           _defaultRenderModeAction;              /** Default render mode action */
    DimensionPickerAction           _defaultMIPDimensionAction;            /** Default MIP dimension action */
};
