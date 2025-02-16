#include "MaterialSettings.h"

#include "TransferFunctionPlugin.h"

#include <QMenu>

using namespace mv::gui;

MaterialSettings::MaterialSettings(QObject* parent, const QString& title) :
    GroupAction(parent, title),
    _transferFunctionPlugin(dynamic_cast<TransferFunctionPlugin*>(parent)),
    _colorPickerAction(this, "CP", qRgb(122, 122, 255))
{
    setConnectionPermissionsToForceNone();

	addAction(&_colorPickerAction);
	_colorPickerAction.initialize(_transferFunctionPlugin);
}

QMenu* MaterialSettings::getContextMenu()
{
    auto menu = new QMenu();

    return menu;
}

void MaterialSettings::fromVariantMap(const QVariantMap& variantMap)
{
    WidgetAction::fromVariantMap(variantMap);

    _colorPickerAction.fromParentVariantMap(variantMap);
}

QVariantMap MaterialSettings::toVariantMap() const
{
    QVariantMap variantMap = WidgetAction::toVariantMap();

    _colorPickerAction.insertIntoVariantMap(variantMap);

    return variantMap;
}
