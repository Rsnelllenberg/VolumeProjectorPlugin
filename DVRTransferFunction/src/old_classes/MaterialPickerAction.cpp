#include "MaterialPickerAction.h"

#include "DVRTransferFunction.h"

#include <PointData/PointData.h>

#include <QMenu>

using namespace mv::gui;

MaterialPickerAction::MaterialPickerAction(QObject* parent, const QString& title) :
    GroupAction(parent, title),
    _transferFunctionPlugin(dynamic_cast<DVRTransferFunction*>(parent)),
{
    setConnectionPermissionsToForceNone();

}

void MaterialPickerAction::fromVariantMap(const QVariantMap& variantMap)
{
    WidgetAction::fromVariantMap(variantMap);

    //_datasetsAction.fromParentVariantMap(variantMap);
}

QVariantMap MaterialPickerAction::toVariantMap() const
{
    QVariantMap variantMap = WidgetAction::toVariantMap();

    //_datasetsAction.insertIntoVariantMap(variantMap);

    return variantMap;
}
