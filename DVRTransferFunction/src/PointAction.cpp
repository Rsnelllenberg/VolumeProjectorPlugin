#include "PointAction.h"

#include "TransferFunctionPlugin.h"
#include "TransferFunctionWidget.h"

#include <DataHierarchyItem.h>

using namespace gui;

PointAction::PointAction(QObject* parent, const QString& title) :
    HorizontalGroupAction(parent, title),
    _transferFunctionPlugin(nullptr),
    _sizeAction(this, "Point size", 0.0, 10.0, DEFAULT_POINT_SIZE),
    _opacityAction(this, "Point opacity", 0.0, 1.0, DEFAULT_POINT_OPACITY)
{
	addAction(&_sizeAction);
	addAction(&_opacityAction);
}

void PointAction::connectToPublicAction(WidgetAction* publicAction, bool recursive)
{
    auto publicPointAction = dynamic_cast<PointAction*>(publicAction);

    Q_ASSERT(publicPointAction != nullptr);

    if (publicPointAction == nullptr)
        return;

    if (recursive) {
        actions().connectPrivateActionToPublicAction(&_sizeAction, &publicPointAction->getSizeAction(), recursive);
        actions().connectPrivateActionToPublicAction(&_opacityAction, &publicPointAction->getOpacityAction(), recursive);
    }

    GroupAction::connectToPublicAction(publicAction, recursive);
}

void PointAction::disconnectFromPublicAction(bool recursive)
{
    if (!isConnected())
        return;

    if (recursive) {
        actions().disconnectPrivateActionFromPublicAction(&_sizeAction, recursive);
        actions().disconnectPrivateActionFromPublicAction(&_opacityAction, recursive);
    }

    GroupAction::disconnectFromPublicAction(recursive);
}

void PointAction::fromVariantMap(const QVariantMap& variantMap)
{
    GroupAction::fromVariantMap(variantMap);

    _sizeAction.fromParentVariantMap(variantMap);
    _opacityAction.fromParentVariantMap(variantMap);
}

QVariantMap PointAction::toVariantMap() const
{
    auto variantMap = GroupAction::toVariantMap();

    _sizeAction.insertIntoVariantMap(variantMap);
    _opacityAction.insertIntoVariantMap(variantMap);

    return variantMap;
}
