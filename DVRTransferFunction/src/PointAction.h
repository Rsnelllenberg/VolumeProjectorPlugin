#pragma once

#include <actions/HorizontalGroupAction.h>
#include "actions/DecimalAction.h"

class TransferFunctionPlugin;

using namespace mv::gui;

class PointAction : public HorizontalGroupAction
{
    Q_OBJECT

public:
    
    /**
     * Construct with \p parent and \p title
     * @param parent Pointer to parent object
     * @param title Title of the action
     */
    Q_INVOKABLE PointAction(QObject* parent, const QString& title);

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

    float getSize() { return _sizeAction.getValue(); }
    float getOpacity() { return _opacityAction.getValue(); }

    DecimalAction& getSizeAction() { return _sizeAction; }
    DecimalAction& getOpacityAction() { return _opacityAction; }

private:
    TransferFunctionPlugin* _transferFunctionPlugin;    /** Pointer to scatterplot plugin */
    DecimalAction            _sizeAction;                /** Point size action */
    DecimalAction            _opacityAction;             /** Point opacity action */

    static constexpr double DEFAULT_POINT_SIZE      = 2.0;     /** Default point size */
    static constexpr double DEFAULT_POINT_OPACITY   = 0.5;     /** Default point opacity */

    friend class TransferFunctionPlugin;
    friend class mv::AbstractActionsManager;
};

Q_DECLARE_METATYPE(PointAction)

inline const auto pointPlotActionMetaTypeId = qRegisterMetaType<PointAction*>("PointAction");