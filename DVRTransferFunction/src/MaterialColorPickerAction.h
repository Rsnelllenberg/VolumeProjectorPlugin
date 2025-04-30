#pragma once

#include "actions/WidgetAction.h"
#include "actions/IntegralAction.h"
#include "InteractiveShape.h"
#include "MaterialTransitionsAction.h"

#include <QVBoxLayout>
#include <QColorDialog>

using namespace mv::gui;
class TransferFunctionPlugin;

/**
    * A modified version of the ColorPickerAction class available in the main ManiVault repository.
    * This version is useable by other classes than the ColorAction class and add gradients.
    *
    * @author Thomas Kroes(original), Ravi Snellenberg
    */
class MaterialColorPickerAction : public WidgetAction
{
    Q_OBJECT
public:
    /**
        * Constructor
        * @param parent Pointer to parent object
        * @param title Title of the action
        * @param color Initial color
        */
    Q_INVOKABLE MaterialColorPickerAction(QObject* parent, const QString& title, const QColor& color = DEFAULT_COLOR);

    /** Gets the current color */
    QColor getColor() const;

    /**
        * Sets the current color
        * @param color Current color
        */
    void setColor(const QColor& color);
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
        * Load plugin from variant map
        * @param variantMap Variant map representation of the plugin
        */
    void fromVariantMap(const QVariantMap& variantMap) override;

    /**
        * Save plugin to variant map
        * @return Variant map representation of the plugin
        */
    QVariantMap toVariantMap() const override;

signals:

    /**
        * Signals that the current color changed
        * @param color Current color that changed
        */
    void colorChanged(const QColor& color);

protected:
    QColor _color;                     /** Current color */


    static const QColor DEFAULT_COLOR; /** Default color */
    TransferFunctionPlugin* _transferFunctionPlugin;    /** Pointer to scatterplot plugin */



    friend class mv::AbstractActionsManager;

    class Widget : public WidgetActionWidget {
    protected:
        Widget(QWidget* parent, MaterialColorPickerAction* colorPickerAction);

    protected:
        QVBoxLayout         _layout;                    /** Main layout */
        QColorDialog        _colorDialog;               /** Non-visible color dialog from which the color picker control is stolen */
        IntegralAction      _hueAction;                 /** Hue action */
        IntegralAction      _saturationAction;          /** Saturation action */
        IntegralAction      _lightnessAction;           /** Lightness action */
        IntegralAction      _alphaAction;               /** Alpha action */
        bool                _updateElements;            /** Whether the color picker action should be updated */

        int maximumHeightColorWidget = 255;
        int maximumHeightHSLWidget = 100;

        friend class MaterialColorPickerAction;
    };

    QWidget* getWidget(QWidget* parent, const std::int32_t& widgetFlags) override {
        return new Widget(parent, this);
    };
};

Q_DECLARE_METATYPE(MaterialColorPickerAction)

inline const auto materialColorPickerActionMetaTypeId = qRegisterMetaType<MaterialColorPickerAction*>("MaterialColorPickerAction*");

