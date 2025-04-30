#pragma once

#include <actions/GroupAction.h>
#include "actions/ToggleAction.h"
#include "actions/IntegralAction.h"
#include "GlobalAlphaAction.h"

#include "MaterialColorPickerAction.h"
#include "GradientPickerAction.h"
#include "MaterialTransitionsAction.h"
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>


using namespace mv::gui;

class TransferFunctionPlugin;

/**
 * Material settings action class
 *
 * Action class for configuring material settings
 */
class MaterialSettings : public GroupAction
{
public:
    /**
     * Construct with \p parent object and \p title
     * @param parent Pointer to parent object
     * @param title Title
     */
    Q_INVOKABLE MaterialSettings(QObject* parent, const QString& title);

    /**
     * Get action context menu
     * @return Pointer to menu
     */
    QMenu* getContextMenu();

public: // Serialization
    /**
     * Load plugin from variant map
     * @param variantMap Variant map representation of the plugin
     */
    void fromVariantMap(const QVariantMap& variantMap) override;

    /**
     * Save plugin to variant map
     * @return Variant map representation of the plugin
     */
    QVariantMap toVariantMap() const override;

public: // Action getters
    MaterialColorPickerAction& getColorBasedColorPickerAction() { return _colorBasedcolorPickerAction; }
    MaterialColorPickerAction& getMaterialBasedColorPickerAction() { return _materialBasedcolorPickerAction; }
    GradientPickerAction& getGradientPickerAction() { return _gradientPickerAction; }
    MaterialTransitionsAction& getMaterialTransitionsAction() { return _materialTransitionsAction; }
	GlobalAlphaAction& getGlobalColorAlphaAction() { return _globalColorAlphaAction; }
	GlobalAlphaAction& getGlobalMaterialAlphaAction() { return _globalMaterialAlphaAction; }

protected:
    TransferFunctionPlugin* _transferFunctionPlugin;    /** Pointer to scatter plot plugin */
    MaterialColorPickerAction _colorBasedcolorPickerAction;       /** Action for picking color */
    MaterialColorPickerAction _materialBasedcolorPickerAction;       /** Action for picking color */
    GradientPickerAction _gradientPickerAction;         /** Action for picking gradient */
    MaterialTransitionsAction _materialTransitionsAction; /** Action for material transitions */
    GlobalAlphaAction _globalColorAlphaAction; 
    GlobalAlphaAction _globalMaterialAlphaAction;


    class Widget : public WidgetActionWidget {
    public:
        Widget(QWidget* parent, MaterialSettings* materialSettingsAction);

    protected:
        QVBoxLayout _layout;                    /** Main layout */
        QTabWidget* _tabWidget;                 /** Tab widget */
        QWidget* _tab1;                         /** First tab */
        QWidget* _tab2;                         /** Second tab */
        QVBoxLayout* _tab1Layout;               /** Layout for first tab */
        QVBoxLayout* _tab2Layout;               /** Layout for second tab */
    };

    QWidget* getWidget(QWidget* parent, const std::int32_t& widgetFlags) override {
        return new Widget(parent, this);
    };
};

