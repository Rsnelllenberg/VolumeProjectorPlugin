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
    _mipDimensionPickerAction(this, "MIP Dimension"),
    _renderModeAction(this, "Render Mode", QStringList{ "MultiDimensional Composite Full", "MultiDimensional Composite 2D Pos", "MultiDimensional Composite Color", "1D MIP"}, "MultiDimensional Composite")
{
    setText("Settings");

    addAction(&_datasetNameAction);
    addAction(&_renderModeAction);
    addAction(&_mipDimensionPickerAction);
    addAction(&_xDimClippingPlaneAction);
    addAction(&_yDimClippingPlaneAction);
    addAction(&_zDimClippingPlaneAction);

    

    _datasetNameAction.setToolTip("Name of currently shown dataset");
    _xDimClippingPlaneAction.setToolTip("X dimension clipping plane");
    _yDimClippingPlaneAction.setToolTip("Y dimension clipping plane");
    _zDimClippingPlaneAction.setToolTip("Z dimension clipping plane");
    _mipDimensionPickerAction.setToolTip("MIP dimension");
    _renderModeAction.setToolTip("Render mode");

    _datasetNameAction.setEnabled(false);
    _datasetNameAction.setText("Dataset name");
    _datasetNameAction.setString(" (No data loaded yet)");

    _xDimClippingPlaneAction.setRange(mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>(_DVRViewPlugin)->getDefaultxDimClippingPlaneAction().getRange());
    _yDimClippingPlaneAction.setRange(mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>(_DVRViewPlugin)->getDefaultyDimClippingPlaneAction().getRange());
    _zDimClippingPlaneAction.setRange(mv::settings().getPluginGlobalSettingsGroupAction<GlobalSettingsAction>(_DVRViewPlugin)->getDefaultzDimClippingPlaneAction().getRange());

    connect(&_xDimClippingPlaneAction, &DecimalRangeAction::rangeChanged, _DVRViewPlugin, &DVRViewPlugin::updatePlot);
    connect(&_yDimClippingPlaneAction, &DecimalRangeAction::rangeChanged, _DVRViewPlugin, &DVRViewPlugin::updatePlot);
    connect(&_zDimClippingPlaneAction, &DecimalRangeAction::rangeChanged, _DVRViewPlugin, &DVRViewPlugin::updatePlot);
    connect(&_mipDimensionPickerAction, &DimensionPickerAction::currentDimensionIndexChanged, _DVRViewPlugin, &DVRViewPlugin::updatePlot);
    connect(&_renderModeAction, &OptionAction::currentIndexChanged, _DVRViewPlugin, &DVRViewPlugin::updatePlot);

}
