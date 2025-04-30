#include "MaterialSettings.h"
#include "TransferFunctionPlugin.h"
#include <QMenu>

using namespace mv::gui;

MaterialSettings::MaterialSettings(QObject* parent, const QString& title) :
    GroupAction(parent, title),
    _transferFunctionPlugin(dynamic_cast<TransferFunctionPlugin*>(parent)),
    _colorBasedcolorPickerAction(this, "CP", qRgb(122, 122, 255)),
    _materialBasedcolorPickerAction(this, "CP", qRgb(122, 122, 255)),
    _gradientPickerAction(this, "GP"),
    _materialTransitionsAction(this, "Material Transitions"),
	_globalColorAlphaAction(this, "Global Color Alpha"),
	_globalMaterialAlphaAction(this, "Global Material Alpha")
{
    setConnectionPermissionsToForceNone();

    _colorBasedcolorPickerAction.initialize(_transferFunctionPlugin);
    _materialBasedcolorPickerAction.initialize(&_materialTransitionsAction);
    _gradientPickerAction.initialize(_transferFunctionPlugin);
    _materialTransitionsAction.initialize(_transferFunctionPlugin);
	_globalColorAlphaAction.initialize(_transferFunctionPlugin);
	_globalMaterialAlphaAction.initialize(&_materialTransitionsAction);
}

QMenu* MaterialSettings::getContextMenu()
{
    auto menu = new QMenu();
    return menu;
}

void MaterialSettings::fromVariantMap(const QVariantMap& variantMap)
{
    WidgetAction::fromVariantMap(variantMap);
    _colorBasedcolorPickerAction.fromParentVariantMap(variantMap);
    _materialBasedcolorPickerAction.fromParentVariantMap(variantMap);
    _gradientPickerAction.fromParentVariantMap(variantMap);
}

QVariantMap MaterialSettings::toVariantMap() const
{
    QVariantMap variantMap = WidgetAction::toVariantMap();
    _colorBasedcolorPickerAction.insertIntoVariantMap(variantMap);
    _materialBasedcolorPickerAction.insertIntoVariantMap(variantMap);
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
    _tabWidget->setMinimumWidth(300);

    // Create the first tab and add actions/UI elements
	_tab1Layout->addWidget(materialSettingsAction->_globalColorAlphaAction.createWidget(_tab1, 100));
    _tab1Layout->addWidget(materialSettingsAction->_colorBasedcolorPickerAction.createWidget(_tab1, 0));
    _tab1Layout->addWidget(materialSettingsAction->_gradientPickerAction.createWidget(_tab1, 0));
    _tab1->setLayout(_tab1Layout);
    _tabWidget->addTab(_tab1, "Color based mode");

    // Create the second tab and add actions/UI elements
	_tab2Layout->addWidget(materialSettingsAction->_globalMaterialAlphaAction.createWidget(_tab2, 100));
    _tab2Layout->addWidget(materialSettingsAction->_materialBasedcolorPickerAction.createWidget(_tab2, 0));
    _tab2Layout->addWidget(materialSettingsAction->_materialTransitionsAction.createWidget(_tab2, 0));
    _tab2->setLayout(_tab2Layout);
    _tabWidget->addTab(_tab2, "Material based mode");

    // Add the tab widget to the main layout
    _layout.addWidget(_tabWidget);
    setLayout(&_layout);
}
