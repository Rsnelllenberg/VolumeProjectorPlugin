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
    _samplerPixelSelectionAction(this, "Sampler selection")
{
    setIcon(mv::Application::getIconFont("FontAwesome").getIcon("mouse-pointer"));
    
    setConfigurationFlag(WidgetAction::ConfigurationFlag::HiddenInActionContextMenu);
    setConfigurationFlag(WidgetAction::ConfigurationFlag::ForceCollapsedInGroup);

    addAction(&_pixelSelectionAction.getTypeAction());
    addAction(&_pixelSelectionAction.getBrushRadiusAction());

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
    }

    GroupAction::connectToPublicAction(publicAction, recursive);
}

void SelectionAction::disconnectFromPublicAction(bool recursive)
{
    if (!isConnected())
        return;

    if (recursive) {
        actions().disconnectPrivateActionFromPublicAction(&_pixelSelectionAction, recursive);
    }

    GroupAction::disconnectFromPublicAction(recursive);
}

void SelectionAction::fromVariantMap(const QVariantMap& variantMap)
{
    GroupAction::fromVariantMap(variantMap);

    _pixelSelectionAction.fromParentVariantMap(variantMap);
    _samplerPixelSelectionAction.fromParentVariantMap(variantMap);
}

QVariantMap SelectionAction::toVariantMap() const
{
    auto variantMap = GroupAction::toVariantMap();

    _pixelSelectionAction.insertIntoVariantMap(variantMap);
    _samplerPixelSelectionAction.insertIntoVariantMap(variantMap);

    return variantMap;
}