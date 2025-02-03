#pragma once

#include <actions/HorizontalGroupAction.h>
#include <actions/DecimalRectangleAction.h>
#include <actions/TriggerAction.h>
#include <actions/ToggleAction.h>

using namespace mv::gui;

class TransferFunctionWidget;

/**
 * Navigation action class
 *
 * Action class for navigating
 *
 * @author Thomas Kroes
 */
class NavigationAction : public HorizontalGroupAction
{
public:

    /**
     * Construct with \p parent and \p title
     * @param parent Pointer to parent object
     * @param title Title of the action
     */
    Q_INVOKABLE NavigationAction(QObject* parent, const QString& title);

    /**
     * Initialize the navigation action with pointer to owning \p transferFunctionWidget
     * @param transferFunctionWidget Pointer to transferFunction widget
     */
    void initialize(TransferFunctionWidget* transferFunctionWidget);

protected: // Linking

    /**
     * Connect this action to a public action
     * @param publicAction Pointer to public action to connect to
     * @param recursive Whether to also connect descendant child actions
     */
    void connectToPublicAction(WidgetAction* publicAction, bool recursive) override;

    /**
     * Disconnect this action from its public action
     * @param recursive Whether to also disconnect descendant child actions
     */
    void disconnectFromPublicAction(bool recursive) override;

public: // Serialization

    /**
     * Load widget action from variant map
     * @param Variant map representation of the widget action
     */
    void fromVariantMap(const QVariantMap& variantMap) override;

    /**
     * Save widget action to variant map
     * @return Variant map representation of the widget action
     */

    QVariantMap toVariantMap() const override;

public: // Action getters

    DecimalRectangleAction& getZoomRectangleAction() { return _zoomRectangleAction; }
    TriggerAction& getZoomDataExtentsAction() { return _zoomDataExtentsAction; }
    ToggleAction& getFreezeZoomAction() { return _freezeZoomAction; }

private:
    TransferFunctionWidget*      _transferFunctionWidget;         /** Pointer to owning transferFunction widget */
    DecimalRectangleAction  _zoomRectangleAction;       /** Rectangle action for setting the current zoom bounds */
    TriggerAction           _zoomDataExtentsAction;     /** Trigger action to zoom to data extents */
    ToggleAction            _freezeZoomAction;          /** Action for toggling the zoom rectangle freeze */

    friend class mv::AbstractActionsManager;
};

Q_DECLARE_METATYPE(NavigationAction)

inline const auto navigationActionMetaTypeId = qRegisterMetaType<NavigationAction*>("NavigationAction");