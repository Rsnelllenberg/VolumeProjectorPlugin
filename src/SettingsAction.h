#pragma once

#include <actions/GroupAction.h>
#include <actions/StringAction.h>
#include <actions/DecimalAction.h>
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
    DecimalAction& getPointSizeAction() { return _pointSizeAction; }
    DecimalAction& getPointOpacityAction() { return _pointOpacityAction; }

private:
    DVRViewPlugin*    _DVRViewPlugin;       /** Pointer to Example OpenGL Viewer Plugin */
    StringAction            _datasetNameAction;         /** Action for displaying the current data set name */
    DecimalAction           _pointSizeAction;           /** point size action */
    DecimalAction           _pointOpacityAction;        /** point opacity action */
};
