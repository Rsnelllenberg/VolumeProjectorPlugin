#include "SettingsAction.h"
#include "GlobalSettingsAction.h"

#include "DVRViewPlugin.h"

#include <QHBoxLayout>

using namespace mv::gui;

SettingsAction::SettingsAction(QObject* parent, const QString& title) :
    GroupAction(parent, title),
    _DVRViewPlugin(dynamic_cast<DVRViewPlugin*>(parent)),
    _datasetNameAction(this, "Dataset Name"),
    _xDimClippingPlaneAction(this, "X Clipping Plane", NumericalRange(0.0f, 1.0f), NumericalRange(0.0f, 1.0f), 5),
    _yDimClippingPlaneAction(this, "Y Clipping Plane", NumericalRange(0.0f, 1.0f), NumericalRange(0.0f, 1.0f), 5),
    _zDimClippingPlaneAction(this, "Z Clipping Plane", NumericalRange(0.0f, 1.0f), NumericalRange(0.0f, 1.0f), 5),
    _useCustomRenderSpaceAction(this, "Use Custom Render Space"),
    _xRenderSizeAction(this, "X Render Size", 0, 500, 50),
    _yRenderSizeAction(this, "Y Render Size", 0, 500, 50),
    _zRenderSizeAction(this, "Z Render Size", 0, 500, 50),
    _mipDimensionPickerAction(this, "MIP Dimension"),
    _renderModeAction(this, "Render Mode", QStringList{ "MaterialTransition Full", "MaterialTransition 2D", "MultiDimensional Composite Full", "MultiDimensional Composite 2D Pos", "MultiDimensional Composite Color", "1D MIP" }, "MultiDimensional Composite")
{
    setText("Settings");

    addAction(&_datasetNameAction);
    addAction(&_renderModeAction);
    addAction(&_mipDimensionPickerAction);

    addAction(&_xDimClippingPlaneAction);
    addAction(&_yDimClippingPlaneAction);
    addAction(&_zDimClippingPlaneAction);

    addAction(&_useCustomRenderSpaceAction);

    addAction(&_xRenderSizeAction);
    addAction(&_yRenderSizeAction);
    addAction(&_zRenderSizeAction);

    _datasetNameAction.setToolTip("Name of currently shown dataset");
    _xDimClippingPlaneAction.setToolTip("X dimension clipping plane");
    _yDimClippingPlaneAction.setToolTip("Y dimension clipping plane");
    _zDimClippingPlaneAction.setToolTip("Z dimension clipping plane");

    _useCustomRenderSpaceAction.setToolTip("Toggle custom render space");

    _xRenderSizeAction.setToolTip("X dimension render size");
    _yRenderSizeAction.setToolTip("Y dimension render size");
    _zRenderSizeAction.setToolTip("Z dimension render size");

    _mipDimensionPickerAction.setToolTip("MIP dimension");
    _renderModeAction.setToolTip("Render mode");

    _datasetNameAction.setEnabled(false);
    _datasetNameAction.setText("Dataset name");
    _datasetNameAction.setString(" (No data loaded yet)");

    _xDimClippingPlaneAction.setRange(mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>(_DVRViewPlugin)->getDefaultxDimClippingPlaneAction().getRange());
    _yDimClippingPlaneAction.setRange(mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>(_DVRViewPlugin)->getDefaultyDimClippingPlaneAction().getRange());
    _zDimClippingPlaneAction.setRange(mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>(_DVRViewPlugin)->getDefaultzDimClippingPlaneAction().getRange());

    _xRenderSizeAction.setRange(mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>(_DVRViewPlugin)->getDefaultxRenderSizeAction().getRange());
    _yRenderSizeAction.setRange(mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>(_DVRViewPlugin)->getDefaultyRenderSizeAction().getRange());
    _zRenderSizeAction.setRange(mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>(_DVRViewPlugin)->getDefaultzRenderSizeAction().getRange());

    connect(&_xDimClippingPlaneAction, &DecimalRangeAction::rangeChanged, _DVRViewPlugin, &DVRViewPlugin::updateRenderSettings);
    connect(&_yDimClippingPlaneAction, &DecimalRangeAction::rangeChanged, _DVRViewPlugin, &DVRViewPlugin::updateRenderSettings);
    connect(&_zDimClippingPlaneAction, &DecimalRangeAction::rangeChanged, _DVRViewPlugin, &DVRViewPlugin::updateRenderSettings);

    connect(&_useCustomRenderSpaceAction, &ToggleAction::toggled, _DVRViewPlugin, &DVRViewPlugin::updateRenderSettings);

    connect(&_xRenderSizeAction, &IntegralAction::valueChanged, _DVRViewPlugin, &DVRViewPlugin::updateRenderSettings);
    connect(&_yRenderSizeAction, &IntegralAction::valueChanged, _DVRViewPlugin, &DVRViewPlugin::updateRenderSettings);
    connect(&_zRenderSizeAction, &IntegralAction::valueChanged, _DVRViewPlugin, &DVRViewPlugin::updateRenderSettings);

    connect(&_mipDimensionPickerAction, &DimensionPickerAction::currentDimensionIndexChanged, _DVRViewPlugin, &DVRViewPlugin::updateRenderSettings);
    connect(&_renderModeAction, &OptionAction::currentIndexChanged, _DVRViewPlugin, &DVRViewPlugin::updateRenderSettings);
}
