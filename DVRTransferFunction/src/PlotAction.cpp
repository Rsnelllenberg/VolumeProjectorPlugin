#include "PlotAction.h"
#include "TransferFunctionPlugin.h"
#include "TransferFunctionWidget.h"

using namespace mv::gui;

PlotAction::PlotAction(QObject* parent, const QString& title) :
    VerticalGroupAction(parent, title),
    _transferFunctionPlugin(nullptr),
    _pointPlotAction(this, "Point"),
    _densityPlotAction(this, "Density")
{
    setToolTip("Plot settings");
    setIcon(mv::Application::getIconFont("FontAwesome").getIcon("paint-brush"));
    setLabelSizingType(LabelSizingType::Auto);

    addAction(&_pointPlotAction.getSizeAction());
    addAction(&_pointPlotAction.getOpacityAction());
    addAction(&_pointPlotAction.getFocusSelection());
    
    addAction(&_densityPlotAction.getSigmaAction());
    addAction(&_densityPlotAction.getContinuousUpdatesAction());
    addAction(&_densityPlotAction.getWeightWithPointSizeAction());
}

void PlotAction::initialize(TransferFunctionPlugin* transferFunctionPlugin)
{
    Q_ASSERT(transferFunctionPlugin != nullptr);

    if (transferFunctionPlugin == nullptr)
        return;

    _transferFunctionPlugin = transferFunctionPlugin;

    _pointPlotAction.initialize(_transferFunctionPlugin);
    _densityPlotAction.initialize(_transferFunctionPlugin);

    auto& transferFunctionWidget = _transferFunctionPlugin->getTransferFunctionWidget();

    const auto updateRenderMode = [this, &transferFunctionWidget]() -> void {
        _pointPlotAction.setVisible(transferFunctionWidget.getRenderMode() == TransferFunctionWidget::SCATTERPLOT);
        _densityPlotAction.setVisible(transferFunctionWidget.getRenderMode() != TransferFunctionWidget::SCATTERPLOT);
    };

    updateRenderMode();

    connect(&transferFunctionWidget, &TransferFunctionWidget::renderModeChanged, this, updateRenderMode);
}

QMenu* PlotAction::getContextMenu()
{
    if (_transferFunctionPlugin == nullptr)
        return nullptr;

    switch (_transferFunctionPlugin->getTransferFunctionWidget().getRenderMode())
    {
        case TransferFunctionWidget::RenderMode::SCATTERPLOT:
            return _pointPlotAction.getContextMenu();
            break;

        case TransferFunctionWidget::RenderMode::DENSITY:
        case TransferFunctionWidget::RenderMode::LANDSCAPE:
            return _densityPlotAction.getContextMenu();
            break;

        default:
            break;
    }

    return new QMenu("Plot");
}

void PlotAction::connectToPublicAction(WidgetAction* publicAction, bool recursive)
{
    auto publicPlotAction = dynamic_cast<PlotAction*>(publicAction);

    Q_ASSERT(publicPlotAction != nullptr);

    if (publicPlotAction == nullptr)
        return;

    if (recursive) {
        actions().connectPrivateActionToPublicAction(&_pointPlotAction, &publicPlotAction->getPointPlotAction(), recursive);
        actions().connectPrivateActionToPublicAction(&_densityPlotAction, &publicPlotAction->getDensityPlotAction(), recursive);
    }

    GroupAction::connectToPublicAction(publicAction, recursive);
}

void PlotAction::disconnectFromPublicAction(bool recursive)
{
    if (!isConnected())
        return;

    if (recursive) {
        actions().disconnectPrivateActionFromPublicAction(&_pointPlotAction, recursive);
        actions().disconnectPrivateActionFromPublicAction(&_densityPlotAction, recursive);
    }

    GroupAction::disconnectFromPublicAction(recursive);
}

void PlotAction::fromVariantMap(const QVariantMap& variantMap)
{
    GroupAction::fromVariantMap(variantMap);

    _pointPlotAction.fromParentVariantMap(variantMap);
    _densityPlotAction.fromParentVariantMap(variantMap);
}

QVariantMap PlotAction::toVariantMap() const
{
    auto variantMap = GroupAction::toVariantMap();

    _pointPlotAction.insertIntoVariantMap(variantMap);
    _densityPlotAction.insertIntoVariantMap(variantMap);

    return variantMap;
}
