#include "NavigationAction.h"
#include "TransferFunctionWidget.h"

#include <Application.h>
#include <CoreInterface.h>

#include <util/Icon.h>

using namespace mv;
using namespace mv::gui;

NavigationAction::NavigationAction(QObject* parent, const QString& title) :
    HorizontalGroupAction(parent, title),
    _transferFunctionWidget(nullptr),
    _zoomRectangleAction(this, "Zoom Rectangle"),
    _zoomDataExtentsAction(this, "Zoom to data extents"),
    _freezeZoomAction(this, "Freeze zoom")
{
    setIcon(mv::Application::getIconFont("FontAwesome").getIcon("image"));
    setShowLabels(false);

    addAction(&_zoomRectangleAction);
    addAction(&_zoomDataExtentsAction);
    addAction(&_freezeZoomAction);

    auto& fontAwesome = Application::getIconFont("FontAwesome");

    _zoomRectangleAction.setToolTip("Extents of the current view");
    _zoomRectangleAction.setIcon(combineIcons(fontAwesome.getIcon("expand"), fontAwesome.getIcon("ellipsis-h")));
    _zoomRectangleAction.setIconByName("vector-square");
    _zoomRectangleAction.setConfigurationFlag(WidgetAction::ConfigurationFlag::ForceCollapsedInGroup);

    _zoomDataExtentsAction.setToolTip("Zoom to the extents of the data (Alt + O)");
    _zoomDataExtentsAction.setIconByName("expand");
    _zoomDataExtentsAction.setDefaultWidgetFlags(TriggerAction::Icon);
    _zoomDataExtentsAction.setConnectionPermissionsToForceNone(true);
    _zoomDataExtentsAction.setShortcutContext(Qt::WidgetWithChildrenShortcut);
    _zoomDataExtentsAction.setShortcut(QKeySequence("Alt+O"));

    _freezeZoomAction.setToolTip("Freeze the zoom extents");
    _freezeZoomAction.setConnectionPermissionsToForceNone(true);
}

void NavigationAction::initialize(TransferFunctionWidget* transferFunctionWidget)
{
    Q_ASSERT(transferFunctionWidget != nullptr);

    if (transferFunctionWidget == nullptr)
        return;

    _transferFunctionWidget = transferFunctionWidget;

    _transferFunctionWidget->addAction(&_zoomDataExtentsAction);

    connect(&_zoomDataExtentsAction, &TriggerAction::triggered, _transferFunctionWidget, &TransferFunctionWidget::resetView);
}

void NavigationAction::connectToPublicAction(WidgetAction* publicAction, bool recursive)
{
    auto publicNavigationAction = dynamic_cast<NavigationAction*>(publicAction);

    Q_ASSERT(publicNavigationAction != nullptr);

    if (publicNavigationAction == nullptr)
        return;

    if (recursive) {
        actions().connectPrivateActionToPublicAction(&_zoomRectangleAction, &publicNavigationAction->getZoomRectangleAction(), recursive);
    }

    HorizontalGroupAction::connectToPublicAction(publicAction, recursive);
}

void NavigationAction::disconnectFromPublicAction(bool recursive)
{
    if (!isConnected())
        return;

    if (recursive) {
        actions().disconnectPrivateActionFromPublicAction(&_zoomRectangleAction, recursive);
    }

    HorizontalGroupAction::disconnectFromPublicAction(recursive);
}

void NavigationAction::fromVariantMap(const QVariantMap& variantMap)
{
    HorizontalGroupAction::fromVariantMap(variantMap);

    _zoomRectangleAction.fromParentVariantMap(variantMap);
    _zoomDataExtentsAction.fromParentVariantMap(variantMap);
    _freezeZoomAction.fromParentVariantMap(variantMap);
}

QVariantMap NavigationAction::toVariantMap() const
{
    auto variantMap = HorizontalGroupAction::toVariantMap();

    _zoomRectangleAction.insertIntoVariantMap(variantMap);
    _zoomDataExtentsAction.insertIntoVariantMap(variantMap);
    _freezeZoomAction.insertIntoVariantMap(variantMap);

    return variantMap;
}
