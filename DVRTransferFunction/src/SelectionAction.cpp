#include "SelectionAction.h"
#include "TransferFunctionPlugin.h"
#include "TransferFunctionWidget.h"

#include <util/PixelSelectionTool.h>

#include <QHBoxLayout>
#include <QPushButton>

using namespace mv::gui;

SelectionAction::SelectionAction(QObject* parent, const QString& title) :
    GroupAction(parent, title),
    _pixelSelectionAction(this, "Point Selection"),
    _samplerPixelSelectionAction(this, "Sampler selection"),
    _displayModeAction(this, "Display mode", { "Outline", "Override" }),
    _outlineOverrideColorAction(this, "Custom color", true)
{
    setIcon(mv::Application::getIconFont("FontAwesome").getIcon("mouse-pointer"));
    
    setConfigurationFlag(WidgetAction::ConfigurationFlag::HiddenInActionContextMenu);
    setConfigurationFlag(WidgetAction::ConfigurationFlag::ForceCollapsedInGroup);

    addAction(&_pixelSelectionAction.getTypeAction());
    addAction(&_pixelSelectionAction.getBrushRadiusAction());
    addAction(&_pixelSelectionAction.getNotifyDuringSelectionAction());
    addAction(&_pixelSelectionAction.getOverlayColorAction());


    _pixelSelectionAction.getOverlayColorAction().setText("Color");

    _displayModeAction.setToolTip("The way in which selection is visualized");

}

void SelectionAction::initialize(TransferFunctionPlugin* transferFunctionPlugin)
{
    Q_ASSERT(transferFunctionPlugin != nullptr);

    if (transferFunctionPlugin == nullptr)
        return;

    auto& transferFunctionWidget = transferFunctionPlugin->getTransferFunctionWidget();

    getPixelSelectionAction().initialize(&transferFunctionWidget, &transferFunctionWidget.getPixelSelectionTool(), {
        PixelSelectionType::Rectangle,
        PixelSelectionType::Brush,
        PixelSelectionType::Lasso,
        PixelSelectionType::Polygon
    });

    getSamplerPixelSelectionAction().initialize(&transferFunctionWidget, &transferFunctionWidget.getSamplerPixelSelectionTool(), {
        PixelSelectionType::Sample
    });

    _displayModeAction.setCurrentIndex(static_cast<std::int32_t>(transferFunctionPlugin->getTransferFunctionWidget().getSelectionDisplayMode()));

    connect(&_displayModeAction, &OptionAction::currentIndexChanged, this, [this, transferFunctionPlugin](const std::int32_t& currentIndex) {
        transferFunctionPlugin->getTransferFunctionWidget().setSelectionDisplayMode(static_cast<PointSelectionDisplayMode>(currentIndex));
    });

    connect(&_outlineOverrideColorAction, &ToggleAction::toggled, this, [this, transferFunctionPlugin](bool toggled) {
        transferFunctionPlugin->getTransferFunctionWidget().setSelectionOutlineOverrideColor(toggled);
    });

    const auto updateReadOnly = [this, transferFunctionPlugin]() -> void {
        setEnabled(transferFunctionPlugin->getPositionDataset().isValid());
    };

    updateReadOnly();

    connect(&transferFunctionPlugin->getPositionDataset(), &Dataset<Points>::changed, this, updateReadOnly);
}

void SelectionAction::connectToPublicAction(WidgetAction* publicAction, bool recursive)
{
    auto publicSelectionAction = dynamic_cast<SelectionAction*>(publicAction);

    Q_ASSERT(publicSelectionAction != nullptr);

    if (publicSelectionAction == nullptr)
        return;

    if (recursive) {
        actions().connectPrivateActionToPublicAction(&_pixelSelectionAction, &publicSelectionAction->getPixelSelectionAction(), recursive);
        actions().connectPrivateActionToPublicAction(&_displayModeAction, &publicSelectionAction->getDisplayModeAction(), recursive);
        actions().connectPrivateActionToPublicAction(&_outlineOverrideColorAction, &publicSelectionAction->getOutlineOverrideColorAction(), recursive);
    }

    GroupAction::connectToPublicAction(publicAction, recursive);
}

void SelectionAction::disconnectFromPublicAction(bool recursive)
{
    if (!isConnected())
        return;

    if (recursive) {
        actions().disconnectPrivateActionFromPublicAction(&_pixelSelectionAction, recursive);
        actions().disconnectPrivateActionFromPublicAction(&_displayModeAction, recursive);
        actions().disconnectPrivateActionFromPublicAction(&_outlineOverrideColorAction, recursive);
    }

    GroupAction::disconnectFromPublicAction(recursive);
}

void SelectionAction::fromVariantMap(const QVariantMap& variantMap)
{
    GroupAction::fromVariantMap(variantMap);

    _pixelSelectionAction.fromParentVariantMap(variantMap);
    _samplerPixelSelectionAction.fromParentVariantMap(variantMap);
    _displayModeAction.fromParentVariantMap(variantMap);
    _outlineOverrideColorAction.fromParentVariantMap(variantMap);
}

QVariantMap SelectionAction::toVariantMap() const
{
    auto variantMap = GroupAction::toVariantMap();

    _pixelSelectionAction.insertIntoVariantMap(variantMap);
    _samplerPixelSelectionAction.insertIntoVariantMap(variantMap);
    _displayModeAction.insertIntoVariantMap(variantMap);
    _outlineOverrideColorAction.insertIntoVariantMap(variantMap);

    return variantMap;
}