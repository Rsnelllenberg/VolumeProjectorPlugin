#include "RenderModeAction.h"
#include "TransferFunctionPlugin.h"
#include "TransferFunctionWidget.h"

using namespace mv::gui;

RenderModeAction::RenderModeAction(QObject* parent, const QString& title) :
    OptionAction(parent, title, { "Scatter", "Density", "Contour" }),
    _transferFunctionPlugin(nullptr),
    _scatterPlotAction(this, "Scatter"),
    _densityPlotAction(this, "Density"),
    _contourPlotAction(this, "Contour")
{
    setIcon(mv::Application::getIconFont("FontAwesome").getIcon("image"));
    setDefaultWidgetFlags(OptionAction::HorizontalButtons);
    setEnabled(false);

    _scatterPlotAction.setConnectionPermissionsToForceNone(true);
    _densityPlotAction.setConnectionPermissionsToForceNone(true);
    _contourPlotAction.setConnectionPermissionsToForceNone(true);

    _scatterPlotAction.setShortcutContext(Qt::WidgetWithChildrenShortcut);
    _densityPlotAction.setShortcutContext(Qt::WidgetWithChildrenShortcut);
    _contourPlotAction.setShortcutContext(Qt::WidgetWithChildrenShortcut);

    _scatterPlotAction.setShortcut(QKeySequence("S"));
    _densityPlotAction.setShortcut(QKeySequence("D"));
    _contourPlotAction.setShortcut(QKeySequence("C"));

    _scatterPlotAction.setToolTip("Set render mode to scatter plot (S)");
    _densityPlotAction.setToolTip("Set render mode to density plot (D)");
    _contourPlotAction.setToolTip("Set render mode to contour plot (C)");
}

void RenderModeAction::initialize(TransferFunctionPlugin* transferFunctionPlugin)
{
    Q_ASSERT(transferFunctionPlugin != nullptr);

    if (transferFunctionPlugin == nullptr)
        return;

    _transferFunctionPlugin = transferFunctionPlugin;

    _transferFunctionPlugin->getWidget().addAction(&_scatterPlotAction);
    _transferFunctionPlugin->getWidget().addAction(&_densityPlotAction);
    _transferFunctionPlugin->getWidget().addAction(&_contourPlotAction);

    const auto currentIndexChanged = [this]() {
        const auto renderMode = static_cast<RenderMode>(getCurrentIndex());

        _scatterPlotAction.setChecked(renderMode == RenderMode::TransferFunction);
        _densityPlotAction.setChecked(renderMode == RenderMode::DensityPlot);
        _contourPlotAction.setChecked(renderMode == RenderMode::ContourPlot);

        _transferFunctionPlugin->getTransferFunctionWidget().setRenderMode(static_cast<TransferFunctionWidget::RenderMode>(getCurrentIndex()));
    };

    currentIndexChanged();

    connect(this, &OptionAction::currentIndexChanged, this, currentIndexChanged);

    connect(&_scatterPlotAction, &QAction::toggled, this, [this, transferFunctionPlugin](bool toggled) {
        if (toggled)
            setCurrentIndex(static_cast<std::int32_t>(RenderMode::TransferFunction));
    });

    connect(&_densityPlotAction, &QAction::toggled, this, [this, transferFunctionPlugin](bool toggled) {
        if (toggled)
            setCurrentIndex(static_cast<std::int32_t>(RenderMode::DensityPlot));
    });

    connect(&_contourPlotAction, &QAction::toggled, this, [this, transferFunctionPlugin](bool toggled) {
        if (toggled)
            setCurrentIndex(static_cast<std::int32_t>(RenderMode::ContourPlot));
    });

    setCurrentIndex(static_cast<std::int32_t>(RenderMode::TransferFunction));

    const auto updateReadOnly = [this]() -> void {
        setEnabled(_transferFunctionPlugin->getPositionDataset().isValid());
    };

    updateReadOnly();

    connect(&_transferFunctionPlugin->getPositionDataset(), &Dataset<Points>::changed, this, updateReadOnly);
}

QMenu* RenderModeAction::getContextMenu()
{
    auto menu = new QMenu("Render mode");

    menu->addAction(&_scatterPlotAction);
    menu->addAction(&_densityPlotAction);
    menu->addAction(&_contourPlotAction);

    return menu;
}

void RenderModeAction::connectToPublicAction(WidgetAction* publicAction, bool recursive)
{
    auto publicRenderModeAction = dynamic_cast<RenderModeAction*>(publicAction);

    Q_ASSERT(publicRenderModeAction != nullptr);

    if (publicRenderModeAction == nullptr)
        return;

    if (recursive) {
        actions().connectPrivateActionToPublicAction(&_scatterPlotAction, &publicRenderModeAction->getTransferFunctionAction(), recursive);
        actions().connectPrivateActionToPublicAction(&_densityPlotAction, &publicRenderModeAction->getDensityPlotAction(), recursive);
        actions().connectPrivateActionToPublicAction(&_contourPlotAction, &publicRenderModeAction->getContourPlotAction(), recursive);
    }

    OptionAction::connectToPublicAction(publicAction, recursive);
}

void RenderModeAction::disconnectFromPublicAction(bool recursive)
{
    if (!isConnected())
        return;

    if (recursive) {
        actions().disconnectPrivateActionFromPublicAction(&_scatterPlotAction, recursive);
        actions().disconnectPrivateActionFromPublicAction(&_densityPlotAction, recursive);
        actions().disconnectPrivateActionFromPublicAction(&_contourPlotAction, recursive);
    }

    OptionAction::disconnectFromPublicAction(recursive);
}

void RenderModeAction::fromVariantMap(const QVariantMap& variantMap)
{
    OptionAction::fromVariantMap(variantMap);

    _scatterPlotAction.fromParentVariantMap(variantMap);
    _densityPlotAction.fromParentVariantMap(variantMap);
    _contourPlotAction.fromParentVariantMap(variantMap);
}

QVariantMap RenderModeAction::toVariantMap() const
{
    auto variantMap = OptionAction::toVariantMap();

    _scatterPlotAction.insertIntoVariantMap(variantMap);
    _densityPlotAction.insertIntoVariantMap(variantMap);
    _contourPlotAction.insertIntoVariantMap(variantMap);

    return variantMap;
}
