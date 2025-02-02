#include "SettingsAction.h"

#include "DVRTransferFunction.h"

#include <PointData/PointData.h>

#include <QMenu>

using namespace mv::gui;

SettingsAction::SettingsAction(QObject* parent, const QString& title) :
    GroupAction(parent, title),
    _transferFunctionPlugin(dynamic_cast<DVRTransferFunction*>(parent)),
    _selectionAction(this, "Selection"),
    _datasetsAction(this, "Datasets")
{
	qDebug() << "SettingsAction: started";
    setConnectionPermissionsToForceNone();
    _selectionAction.initialize(_transferFunctionPlugin);
	qDebug() << "SettingsAction: created";
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
