#include "SettingsAction.h"

#include "TransferFunctionPlugin.h"

#include <PointData/PointData.h>

#include <QMenu>

using namespace mv::gui;

SettingsAction::SettingsAction(QObject* parent, const QString& title) :
    GroupAction(parent, title),
    _transferFunctionPlugin(dynamic_cast<TransferFunctionPlugin*>(parent)),
    _selectionAction(this, "Selection"),
    _datasetsAction(this, "Datasets"),
	_pointsAction(this, "Point Parameters")
{
    setConnectionPermissionsToForceNone();
    _selectionAction.initialize(_transferFunctionPlugin);


}

QMenu* SettingsAction::getContextMenu()
{
    auto menu = new QMenu();

    return menu;
}

void SettingsAction::fromVariantMap(const QVariantMap& variantMap)
{
    WidgetAction::fromVariantMap(variantMap);

    _datasetsAction.fromParentVariantMap(variantMap);
    _selectionAction.fromParentVariantMap(variantMap);
}

QVariantMap SettingsAction::toVariantMap() const
{
    QVariantMap variantMap = WidgetAction::toVariantMap();

    _datasetsAction.insertIntoVariantMap(variantMap);
    _selectionAction.insertIntoVariantMap(variantMap);

    return variantMap;
}
