#include "MaterialSettings.h"
#include "TransferFunctionPlugin.h"
#include <QMenu>

using namespace mv::gui;

MaterialSettings::MaterialSettings(QObject* parent, const QString& title) :
    GroupAction(parent, title),
    _transferFunctionPlugin(dynamic_cast<TransferFunctionPlugin*>(parent)),
    _colorPickerAction(this, "CP", qRgb(122, 122, 255)),
    _gradientPickerAction(this, "GP")
{
    setConnectionPermissionsToForceNone();

    addAction(&_colorPickerAction, 2);
    addAction(&_gradientPickerAction, 2);
    _colorPickerAction.initialize(_transferFunctionPlugin);
    _gradientPickerAction.initialize(_transferFunctionPlugin);
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
    _gradientPickerAction.fromParentVariantMap(variantMap);
}

QVariantMap MaterialSettings::toVariantMap() const
{
    QVariantMap variantMap = WidgetAction::toVariantMap();
    _colorPickerAction.insertIntoVariantMap(variantMap);
    _gradientPickerAction.insertIntoVariantMap(variantMap);
    return variantMap;
}

MaterialSettings::Widget::Widget(QWidget* parent, MaterialSettings* materialSettingsAction) :
    WidgetActionWidget(parent, materialSettingsAction),
    _layout(),
    _tabWidget(new QTabWidget()),
    _tab1(new QWidget()),
    _tab2(new QWidget()),
    _tab1Layout(new QVBoxLayout()),
    _tab2Layout(new QVBoxLayout())
{
    // Create the first tab and add actions/UI elements
    _tab1Layout->addWidget(materialSettingsAction->_colorPickerAction.createWidget(_tab1, 0));
    _tab1->setLayout(_tab1Layout);
    _tabWidget->addTab(_tab1, "Color Picker");

    // Create the second tab and add actions/UI elements
    _tab2Layout->addWidget(materialSettingsAction->_gradientPickerAction.createWidget(_tab2, 0));
    _tab2->setLayout(_tab2Layout);
    _tabWidget->addTab(_tab2, "Gradient Picker");

    // Add the tab widget to the main layout
    _layout.addWidget(_tabWidget);
    setLayout(&_layout);
}


