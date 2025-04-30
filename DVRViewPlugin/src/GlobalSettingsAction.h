#pragma once

#include <PluginGlobalSettingsGroupAction.h>
#include "GlobalSettingsAction.h"
#include <actions/DecimalRangeAction.h>
#include <actions/IntegralRangeAction.h>
#include <actions/ToggleAction.h>
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

    mv::gui::IntegralAction& getDefaultxRenderSizeAction() { return _defaultXRenderSizeAction; }
    mv::gui::IntegralAction& getDefaultyRenderSizeAction() { return _defaultYRenderSizeAction; }
    mv::gui::IntegralAction& getDefaultzRenderSizeAction() { return _defaultZRenderSizeAction; }

    mv::gui::IntegralAction& getDefaultRenderCubeSizeAction() { return _defaultRenderCubeSizeAction; }

    mv::gui::DecimalAction& getDefaultStepSizeAction() { return _defaultStepSizeAction; }

    mv::gui::ToggleAction& getDefaultUseShadingAction() { return _defaultUseShadingAction; }
    mv::gui::ToggleAction& getDefaultUseEmptySpaceSkippingAction() { return _defaultUseEmptySpaceSkippingAction; }
    mv::gui::ToggleAction& getDefaultUseCustomRenderSpaceAction() { return _defaultUseCustomRenderSpaceAction; }

    mv::gui::OptionAction& getDefaultRenderModeAction() { return _defaultRenderModeAction; }
    DimensionPickerAction& getDefaultMIPDimensionAction() { return _defaultMIPDimensionAction; }

private:
    mv::gui::DecimalRangeAction     _defaultXDimClippingPlaneAction;       /** Default range size action */
    mv::gui::DecimalRangeAction     _defaultYDimClippingPlaneAction;       /** Default range size action */
    mv::gui::DecimalRangeAction     _defaultZDimClippingPlaneAction;       /** Default range size action */

    mv::gui::DecimalAction          _defaultStepSizeAction;                /** Default step size action */

    mv::gui::IntegralAction         _defaultXRenderSizeAction;             /** Default render size action */
    mv::gui::IntegralAction         _defaultYRenderSizeAction;             /** Default render size action */
    mv::gui::IntegralAction         _defaultZRenderSizeAction;             /** Default render size action */

    mv::gui::IntegralAction         _defaultRenderCubeSizeAction;          /** Default render cube size action */

    mv::gui::ToggleAction           _defaultUseShadingAction;              /** Default toggle action for shading */
    mv::gui::ToggleAction           _defaultUseEmptySpaceSkippingAction;   /** Default toggle action for empty space skipping */
    mv::gui::ToggleAction           _defaultUseCustomRenderSpaceAction;    /** Default toggle action for custom render space */

    mv::gui::OptionAction           _defaultRenderModeAction;              /** Default render mode action, it contains these options "MaterialTransition Full", "MaterialTransition 2D", "NN MaterialTransition", "Alt NN MaterialTransition", "Smooth NN MaterialTransition", "MultiDimensional Composite Full", "MultiDimensional Composite 2D Pos", "MultiDimensional Composite Color", "NN MultiDimensional Composite", "1D MIP" */
    DimensionPickerAction           _defaultMIPDimensionAction;            /** Default MIP dimension action */
};
