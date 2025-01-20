#pragma once

#include <actions/GroupAction.h>
#include <actions/StringAction.h>
#include <actions/DecimalRangeAction.h>
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
    DecimalRangeAction& getXDimensionPickerAction() { return _xDimClippingPlaneAction; }
    DecimalRangeAction& getYDimensionPickerAction() { return _yDimClippingPlaneAction; }
    DecimalRangeAction& getZDimensionPickerAction() { return _zDimClippingPlaneAction; }

private:
    DVRViewPlugin*          _DVRViewPlugin;                     /** Pointer to Example OpenGL Viewer Plugin */
    StringAction            _datasetNameAction;                 /** Action for displaying the current data set name */
    DecimalRangeAction      _xDimClippingPlaneAction;           /** x-dimension range slider for the clipping planes */
    DecimalRangeAction      _yDimClippingPlaneAction;           /** y-dimension range slider for the clipping planes */
    DecimalRangeAction      _zDimClippingPlaneAction;           /** z-dimension range slider for the clipping planes */
};
