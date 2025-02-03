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
    _outlineOverrideColorAction(this, "Custom color", true),
    _outlineScaleAction(this, "Scale", 100.0f, 500.0f, 200.0f, 1),
    _outlineOpacityAction(this, "Opacity", 0.0f, 100.0f, 100.0f, 1),
    _outlineHaloEnabledAction(this, "Halo")
{
    setIcon(mv::Application::getIconFont("FontAwesome").getIcon("mouse-pointer"));
    
    setConfigurationFlag(WidgetAction::ConfigurationFlag::HiddenInActionContextMenu);
    setConfigurationFlag(WidgetAction::ConfigurationFlag::ForceCollapsedInGroup);

    addAction(&_pixelSelectionAction.getTypeAction());
    addAction(&_pixelSelectionAction.getBrushRadiusAction());
    addAction(&_pixelSelectionAction.getModifierAction(), OptionAction::HorizontalButtons);
    addAction(&_pixelSelectionAction.getSelectAction());
    addAction(&_pixelSelectionAction.getNotifyDuringSelectionAction());
    addAction(&_pixelSelectionAction.getOverlayColorAction());

    addAction(&getDisplayModeAction());
    addAction(&getOutlineScaleAction());
    addAction(&getOutlineOpacityAction());
    addAction(&getOutlineHaloEnabledAction());

    _pixelSelectionAction.getOverlayColorAction().setText("Color");

    _displayModeAction.setToolTip("The way in which selection is visualized");

    _outlineScaleAction.setSuffix("%");
    _outlineOpacityAction.setSuffix("%");

    const auto updateActionsReadOnly = [this]() -> void {
        const auto isOutline = static_cast<PointSelectionDisplayMode>(_displayModeAction.getCurrentIndex()) == PointSelectionDisplayMode::Outline;

        _outlineScaleAction.setEnabled(isOutline);
        _outlineOpacityAction.setEnabled(isOutline);
        _outlineHaloEnabledAction.setEnabled(isOutline);
    };

    updateActionsReadOnly();

    connect(&_displayModeAction, &OptionAction::currentIndexChanged, this, updateActionsReadOnly);
    connect(&_outlineOverrideColorAction, &ToggleAction::toggled, this, updateActionsReadOnly);
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
    _outlineScaleAction.setValue(100.0f * transferFunctionPlugin->getTransferFunctionWidget().getSelectionOutlineScale());
    _outlineOpacityAction.setValue(100.0f * transferFunctionPlugin->getTransferFunctionWidget().getSelectionOutlineOpacity());

    _outlineHaloEnabledAction.setChecked(transferFunctionPlugin->getTransferFunctionWidget().getSelectionOutlineHaloEnabled());
    _outlineOverrideColorAction.setChecked(transferFunctionPlugin->getTransferFunctionWidget().getSelectionOutlineOverrideColor());

    connect(&_pixelSelectionAction.getSelectAllAction(), &QAction::triggered, [this, transferFunctionPlugin]() {
        if (transferFunctionPlugin->getPositionDataset().isValid())
            transferFunctionPlugin->getPositionDataset()->selectAll();
    });

    connect(&_pixelSelectionAction.getClearSelectionAction(), &QAction::triggered, this, [this, transferFunctionPlugin]() {
        if (transferFunctionPlugin->getPositionDataset().isValid())
            transferFunctionPlugin->getPositionDataset()->selectNone();
    });

    connect(&_pixelSelectionAction.getInvertSelectionAction(), &QAction::triggered, this, [this, transferFunctionPlugin]() {
        if (transferFunctionPlugin->getPositionDataset().isValid())
            transferFunctionPlugin->getPositionDataset()->selectInvert();
    });

    connect(&_outlineScaleAction, &DecimalAction::valueChanged, this, [this, transferFunctionPlugin](float value) {
        transferFunctionPlugin->getTransferFunctionWidget().setSelectionOutlineScale(0.01f * value);
    });

    connect(&_outlineOpacityAction, &DecimalAction::valueChanged, this, [this, transferFunctionPlugin](float value) {
        transferFunctionPlugin->getTransferFunctionWidget().setSelectionOutlineOpacity(0.01f * value);
    });

    connect(&_outlineHaloEnabledAction, &ToggleAction::toggled, this, [this, transferFunctionPlugin](bool toggled) {
        transferFunctionPlugin->getTransferFunctionWidget().setSelectionOutlineHaloEnabled(toggled);
    });

    connect(&_pixelSelectionAction.getOverlayColorAction(), &ColorAction::colorChanged, this, [this, transferFunctionPlugin](const QColor& color) {
        transferFunctionPlugin->getTransferFunctionWidget().setSelectionOutlineColor(color);
    });

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
        actions().connectPrivateActionToPublicAction(&_outlineScaleAction, &publicSelectionAction->getOutlineScaleAction(), recursive);
        actions().connectPrivateActionToPublicAction(&_outlineOpacityAction, &publicSelectionAction->getOutlineOpacityAction(), recursive);
        actions().connectPrivateActionToPublicAction(&_outlineHaloEnabledAction, &publicSelectionAction->getOutlineHaloEnabledAction(), recursive);
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
        actions().disconnectPrivateActionFromPublicAction(&_outlineScaleAction, recursive);
        actions().disconnectPrivateActionFromPublicAction(&_outlineOpacityAction, recursive);
        actions().disconnectPrivateActionFromPublicAction(&_outlineHaloEnabledAction, recursive);
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
    _outlineScaleAction.fromParentVariantMap(variantMap);
    _outlineOpacityAction.fromParentVariantMap(variantMap);
    _outlineHaloEnabledAction.fromParentVariantMap(variantMap);
}

QVariantMap SelectionAction::toVariantMap() const
{
    auto variantMap = GroupAction::toVariantMap();

    _pixelSelectionAction.insertIntoVariantMap(variantMap);
    _samplerPixelSelectionAction.insertIntoVariantMap(variantMap);
    _displayModeAction.insertIntoVariantMap(variantMap);
    _outlineOverrideColorAction.insertIntoVariantMap(variantMap);
    _outlineScaleAction.insertIntoVariantMap(variantMap);
    _outlineOpacityAction.insertIntoVariantMap(variantMap);
    _outlineHaloEnabledAction.insertIntoVariantMap(variantMap);

    return variantMap;
}