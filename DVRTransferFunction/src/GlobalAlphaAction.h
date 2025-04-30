#pragma once

#include <actions/GroupAction.h>
#include <actions/ToggleAction.h>
#include <actions/IntegralAction.h>

#include "MaterialTransitionsAction.h"

class TransferFunctionPlugin;

using namespace mv::gui;

class GlobalAlphaAction : public GroupAction
{
    Q_OBJECT

public:
    /**
     * Construct with \p parent object and \p title
     * @param parent Pointer to parent object
     * @param title Title
     */
    Q_INVOKABLE GlobalAlphaAction(QObject* parent, const QString& title);

    /**
     * Initialize the global alpha action with \p transferFunctionPlugin
     * @param transferFunctionPlugin Pointer to transferFunction plugin
     */
    void initialize(TransferFunctionPlugin* transferFunctionPlugin);

	void initialize(MaterialTransitionsAction* materialTransitionsAction);

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
     * Load global alpha action from variant
     * @param variantMap Variant representation of the global alpha action
     */
    void fromVariantMap(const QVariantMap& variantMap) override;

    /**
     * Save global alpha action to variant
     * @return Variant representation of the global alpha action
     */
    QVariantMap toVariantMap() const override;

    QWidget* createWidget(QWidget* parent, int maxHeight);

public: // Action getters

    ToggleAction& getAlphaToggleAction() { return _globalAlphaToggle; }
    IntegralAction& getAlphaIntegerAction() { return _globalAlphaValue; }

private:
    ToggleAction    _globalAlphaToggle;
    IntegralAction  _globalAlphaValue;        

    friend class mv::AbstractActionsManager;
};

Q_DECLARE_METATYPE(GlobalAlphaAction)

inline const auto globalAlphaActionMetaTypeId = qRegisterMetaType<GlobalAlphaAction*>("GlobalAlphaAction");
