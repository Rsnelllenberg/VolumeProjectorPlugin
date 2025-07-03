#pragma once

#include <actions/GroupAction.h>
#include <actions/StringAction.h>
#include <actions/DecimalRangeAction.h>
#include <actions/IntegralAction.h>
#include <actions/ToggleAction.h>
#include <PointData/DimensionPickerAction.h>

using namespace mv::gui;

class DVRViewPlugin;

/**
 * Settings action class
 *
 * Action class for configuring settings
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

public: // Action getters

    StringAction& getDatasetNameAction() { return _datasetNameAction; }
    DecimalRangeAction& getXDimClippingPlaneAction() { return _xDimClippingPlaneAction; }
    DecimalRangeAction& getYDimClippingPlaneAction() { return _yDimClippingPlaneAction; }
    DecimalRangeAction& getZDimClippingPlaneAction() { return _zDimClippingPlaneAction; }

    DecimalAction& getStepSizeAction() { return _stepSizeAction; }

    IntegralAction& getXRenderSizeAction() { return _xRenderSizeAction; }
    IntegralAction& getYRenderSizeAction() { return _yRenderSizeAction; }
    IntegralAction& getZRenderSizeAction() { return _zRenderSizeAction; }

    IntegralAction& getRenderCubeSizeAction() { return _renderCubeSizeAction; }

    ToggleAction& getUseShaderAction() { return _useShadingAction; }
    ToggleAction& getUseClutterRemoverAction() { return _useClutterRemover; }
    ToggleAction& getUseCustomRenderSpaceAction() { return _useCustomRenderSpaceAction; }

    DimensionPickerAction& getMIPDimensionPickerAction() { return _mipDimensionPickerAction; }
    OptionAction& getRenderModeAction() { return _renderModeAction; }


private:
    DVRViewPlugin*          _DVRViewPlugin;                     /** Pointer to Example OpenGL Viewer Plugin */
    StringAction            _datasetNameAction;                 /** Action for displaying the current data set name */
    DecimalRangeAction      _xDimClippingPlaneAction;           /** x-dimension range slider for the clipping planes */
    DecimalRangeAction      _yDimClippingPlaneAction;           /** y-dimension range slider for the clipping planes */
    DecimalRangeAction      _zDimClippingPlaneAction;           /** z-dimension range slider for the clipping planes */

    DecimalAction           _stepSizeAction;                    /** Ray stepsize action */

    IntegralAction          _xRenderSizeAction;                 /** x-dimension render size action */
    IntegralAction          _yRenderSizeAction;                 /** y-dimension render size action */
    IntegralAction          _zRenderSizeAction;                 /** z-dimension render size action */

    IntegralAction          _renderCubeSizeAction;              /** Sets the size of the cubes used for empty space skipping action */

    ToggleAction            _useShadingAction;                  /** Toggle action for using shading when available in the render mode */
    ToggleAction            _useClutterRemover;           /** Toggle action for using isolated voxel remover, which skips rendering of isolated voxels to remve clutter */
    ToggleAction            _useCustomRenderSpaceAction;        /** Toggle action for custom render space */

    DimensionPickerAction   _mipDimensionPickerAction;          /** Dimension picker action */
    OptionAction            _renderModeAction;                  /** Render mode action, contains: "MaterialTransition Full", "MaterialTransition 2D", "NN MaterialTransition", "Alt NN MaterialTransition", "Smooth NN MaterialTransition", "MultiDimensional Composite Full", "MultiDimensional Composite 2D Pos", "MultiDimensional Composite Color", "NN MultiDimensional Composite", "1D MIP" */
};
