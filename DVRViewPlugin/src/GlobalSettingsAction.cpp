#include "GlobalSettingsAction.h"

#include <QHBoxLayout>

using namespace mv;
using namespace mv::gui;
using namespace mv::util;

GlobalSettingsAction::GlobalSettingsAction(QObject* parent, const plugin::PluginFactory* pluginFactory) :
    PluginGlobalSettingsGroupAction(parent, pluginFactory),
    _defaultXDimClippingPlaneAction(this, "X Clipping Plane", NumericalRange(0.0f, 1.0f), NumericalRange(0.0f, 1.0f), 1000),
    _defaultYDimClippingPlaneAction(this, "Y Clipping Plane", NumericalRange(0.0f, 1.0f), NumericalRange(0.0f, 1.0f), 1000),
    _defaultZDimClippingPlaneAction(this, "Z Clipping Plane", NumericalRange(0.0f, 1.0f), NumericalRange(0.0f, 1.0f), 1000),
    _defaultStepSizeAction(this, "Step Size", 0.1f, 5.0f, 1.0f),
    _defaultXRenderSizeAction(this, "X Render Size", 0, 500, 50),
    _defaultYRenderSizeAction(this, "Y Render Size", 0, 500, 50),
    _defaultZRenderSizeAction(this, "Z Render Size", 0, 500, 50),
    _defaultRenderCubeSizeAction(this, "Render Cube Size", 1, 500, 30),
    _defaultUseShadingAction(this, "Use Shader"),
    _defaultUseEmptySpaceSkippingAction(this, "Use Empty Space Skipping"),
    _defaultUseCustomRenderSpaceAction(this, "Use Custom Render Space"),
    _defaultRenderModeAction(this, "Render Mode", QStringList{ "MaterialTransition Full", "MaterialTransition 2D", "NN MaterialTransition", "Alt NN MaterialTransition", "Smooth NN MaterialTransition", "MultiDimensional Composite Full", "MultiDimensional Composite 2D Pos", "MultiDimensional Composite Color", "NN MultiDimensional Composite", "1D MIP" }, "MultiDimensional Composite Color"),
    _defaultMIPDimensionAction(this, "MIP Dimension")
{
    _defaultXDimClippingPlaneAction.setToolTip("Default size of the clipping plane range in the x-axis");
    _defaultYDimClippingPlaneAction.setToolTip("Default size of the clipping plane range in the y-axis");
    _defaultZDimClippingPlaneAction.setToolTip("Default size of the clipping plane range in the z-axis");

    _defaultUseShadingAction.setToolTip("Toggle shading");
    _defaultUseEmptySpaceSkippingAction.setToolTip("Toggle empty space skipping");
    _defaultUseCustomRenderSpaceAction.setToolTip("Toggle custom render space");

    _defaultXRenderSizeAction.setToolTip("Default render size in the x-axis");
    _defaultYRenderSizeAction.setToolTip("Default render size in the y-axis");
    _defaultZRenderSizeAction.setToolTip("Default render size in the z-axis");
    
    _defaultRenderCubeSizeAction.setToolTip("Default size of the render cube");

    _defaultRenderModeAction.setToolTip("Default render mode");
    _defaultMIPDimensionAction.setToolTip("Default MIP dimension");

    addAction(&_defaultUseEmptySpaceSkippingAction);
    addAction(&_defaultRenderCubeSizeAction);

    addAction(&_defaultUseShadingAction);
    addAction(&_defaultRenderModeAction);
    addAction(&_defaultMIPDimensionAction);

    addAction(&_defaultXDimClippingPlaneAction);
    addAction(&_defaultYDimClippingPlaneAction);
    addAction(&_defaultZDimClippingPlaneAction);

    addAction(&_defaultUseCustomRenderSpaceAction);

    addAction(&_defaultXRenderSizeAction);
    addAction(&_defaultYRenderSizeAction);
    addAction(&_defaultZRenderSizeAction);
}
