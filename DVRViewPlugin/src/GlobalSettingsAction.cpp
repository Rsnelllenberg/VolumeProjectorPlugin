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
    _defaultRenderModeAction(this, "Render Mode", QStringList{ "MultiDimensional Composite", "1D MIP" }, "MultiDimensional Composite"),
    _defaultMIPDimensionAction(this, "MIP Dimension")
{
    _defaultXDimClippingPlaneAction.setToolTip("Default size of the clipping plane range in the x-axis");
    _defaultYDimClippingPlaneAction.setToolTip("Default size of the clipping plane range in the y-axis");
    _defaultZDimClippingPlaneAction.setToolTip("Default size of the clipping plane range in the z-axis");

    _defaultRenderModeAction.setToolTip("Default render mode");
    _defaultMIPDimensionAction.setToolTip("Default MIP dimension");

    // The add action automatically assigns a settings prefix to _pointSizeAction so there is no need to do this manually
    addAction(&_defaultRenderModeAction);
    addAction(&_defaultMIPDimensionAction);

    addAction(&_defaultXDimClippingPlaneAction);
    addAction(&_defaultYDimClippingPlaneAction);
    addAction(&_defaultZDimClippingPlaneAction);

}
