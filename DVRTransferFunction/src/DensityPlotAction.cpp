#include "DensityPlotAction.h"
#include "TransferFunctionPlugin.h"
#include "TransferFunctionWidget.h"

using namespace mv::gui;

DensityPlotAction::DensityPlotAction(QObject* parent, const QString& title) :
    VerticalGroupAction(parent, title),
    _transferFunctionPlugin(nullptr),
    _sigmaAction(this, "Sigma", 0.01f, 0.5f, DEFAULT_SIGMA, 3),
    _continuousUpdatesAction(this, "Live Updates", DEFAULT_CONTINUOUS_UPDATES),
    _weightWithPointSizeAction(this, "Weight by size", false)
{
    setToolTip("Density plot settings");
    setConfigurationFlag(WidgetAction::ConfigurationFlag::NoLabelInGroup);
    setLabelSizingType(LabelSizingType::Auto);

    addAction(&_sigmaAction);
    addAction(&_continuousUpdatesAction);
    addAction(&_weightWithPointSizeAction);
}

void DensityPlotAction::initialize(TransferFunctionPlugin* transferFunctionPlugin)
{
  
}

QMenu* DensityPlotAction::getContextMenu()
{
    if (_transferFunctionPlugin == nullptr)
        return nullptr;

    auto menu = new QMenu("Plot settings");

    const auto addActionToMenu = [menu](QAction* action) {
        auto actionMenu = new QMenu(action->text());

        actionMenu->addAction(action);

        menu->addMenu(actionMenu);
    };

    addActionToMenu(&_sigmaAction);
    addActionToMenu(&_continuousUpdatesAction);

    return menu;
}

void DensityPlotAction::setVisible(bool visible)
{
    _sigmaAction.setVisible(visible);
    _weightWithPointSizeAction.setVisible(visible);
    _continuousUpdatesAction.setVisible(visible);
}

void DensityPlotAction::connectToPublicAction(WidgetAction* publicAction, bool recursive)
{
    auto publicDensityPlotAction = dynamic_cast<DensityPlotAction*>(publicAction);

    Q_ASSERT(publicDensityPlotAction != nullptr);

    if (publicDensityPlotAction == nullptr)
        return;

    if (recursive) {
        actions().connectPrivateActionToPublicAction(&_sigmaAction, &publicDensityPlotAction->getSigmaAction(), recursive);
        actions().connectPrivateActionToPublicAction(&_weightWithPointSizeAction, &publicDensityPlotAction->getContinuousUpdatesAction(), recursive);
        actions().connectPrivateActionToPublicAction(&_continuousUpdatesAction, &publicDensityPlotAction->getContinuousUpdatesAction(), recursive);
    }

    GroupAction::connectToPublicAction(publicAction, recursive);
}

void DensityPlotAction::disconnectFromPublicAction(bool recursive)
{
    if (!isConnected())
        return;

    if (recursive) {
        actions().disconnectPrivateActionFromPublicAction(&_sigmaAction, recursive);
        actions().disconnectPrivateActionFromPublicAction(&_weightWithPointSizeAction, recursive);
        actions().disconnectPrivateActionFromPublicAction(&_continuousUpdatesAction, recursive);
    }

    GroupAction::disconnectFromPublicAction(recursive);
}

void DensityPlotAction::fromVariantMap(const QVariantMap& variantMap)
{
    GroupAction::fromVariantMap(variantMap);

    _sigmaAction.fromParentVariantMap(variantMap);
    _weightWithPointSizeAction.fromParentVariantMap(variantMap);
    _continuousUpdatesAction.fromParentVariantMap(variantMap);
}

QVariantMap DensityPlotAction::toVariantMap() const
{
    auto variantMap = GroupAction::toVariantMap();

    _sigmaAction.insertIntoVariantMap(variantMap);
    _weightWithPointSizeAction.insertIntoVariantMap(variantMap);
    _continuousUpdatesAction.insertIntoVariantMap(variantMap);

    return variantMap;
}
