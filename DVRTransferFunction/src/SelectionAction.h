#pragma once

#include <actions/GroupAction.h>
#include <actions/PixelSelectionAction.h>
#include <PointData/PointData.h>

class DVRTransferFunction;

using namespace mv::gui;

class SelectionAction : public GroupAction
{
    Q_OBJECT

public:
    
    /**
     * Construct with \p parent object and \p title
     * @param parent Pointer to parent object
     * @param title Title
     */
    Q_INVOKABLE SelectionAction(QObject* parent, const QString& title);

    /**
     * Initialize the selection action with \p dvrTransferFunction
     * @param dvrTransferFunction Pointer to scatterplot plugin
     */
    void initialize(DVRTransferFunction* dvrTransferFunction);

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
     * Load selection action from variant
     * @param Variant representation of the selection action
     */
    void fromVariantMap(const QVariantMap& variantMap) override;

    /**
     * Save selection action to variant
     * @return Variant representation of the selection action
     */
    QVariantMap toVariantMap() const override;

public: // Action getters
    
    PixelSelectionAction& getPixelSelectionAction() { return _pixelSelectionAction; }
    PixelSelectionAction& getSamplerPixelSelectionAction() { return _samplerPixelSelectionAction; }
    OptionAction& getDisplayModeAction() { return _displayModeAction; }


private:
    PixelSelectionAction    _pixelSelectionAction;          /** Pixel selection action */
    PixelSelectionAction    _samplerPixelSelectionAction;   /** Pixel selection action */
    OptionAction            _displayModeAction;             /** Type of selection display (e.g. outline or override) */

	mv::Dataset<Points>*    _pointsDataset;					/** Pointer to the points dataset, prevents a crash during startup  */

    friend class mv::AbstractActionsManager;
};

Q_DECLARE_METATYPE(SelectionAction)

inline const auto selectionActionMetaTypeId = qRegisterMetaType<SelectionAction*>("SelectionAction");