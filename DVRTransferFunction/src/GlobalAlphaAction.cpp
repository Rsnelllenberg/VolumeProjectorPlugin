#include "GlobalAlphaAction.h"
#include "TransferFunctionPlugin.h"
#include "TransferFunctionWidget.h"

#include <QHBoxLayout>
#include <QPushButton>

using namespace mv::gui;

GlobalAlphaAction::GlobalAlphaAction(QObject* parent, const QString& title) :
    GroupAction(parent, title),
    _globalAlphaToggle(this, "Global Alpha Toggle Action"),
    _globalAlphaValue(this, "Global Alpha Value Action", 0, 255, 100)
{

    addAction(&_globalAlphaToggle);
    addAction(&_globalAlphaValue);
}

void GlobalAlphaAction::initialize(TransferFunctionPlugin* transferFunctionPlugin)
{
    Q_ASSERT(transferFunctionPlugin != nullptr);

    if (transferFunctionPlugin == nullptr)
        return;

    TransferFunctionWidget& widget = transferFunctionPlugin->getTransferFunctionWidget();

	connect(&_globalAlphaToggle, &ToggleAction::toggled, &widget, &TransferFunctionWidget::setGlobalAlphaToggle);
	connect(&_globalAlphaValue, &IntegralAction::valueChanged, &widget, &TransferFunctionWidget::setGlobalAlphaValue);
}

void GlobalAlphaAction::initialize(MaterialTransitionsAction* materialTransitionsAction)
{
	Q_ASSERT(materialTransitionsAction != nullptr);

	if (materialTransitionsAction == nullptr)
		return;

	connect(&_globalAlphaToggle, &ToggleAction::toggled, materialTransitionsAction, &MaterialTransitionsAction::setUseGlobalAlpha);
	connect(&_globalAlphaValue, &IntegralAction::valueChanged, materialTransitionsAction, &MaterialTransitionsAction::setGlobalAlphaValue);
}

void GlobalAlphaAction::connectToPublicAction(WidgetAction* publicAction, bool recursive)
{
    auto publicGlobalAlphaAction = dynamic_cast<GlobalAlphaAction*>(publicAction);

    Q_ASSERT(publicGlobalAlphaAction != nullptr);

    if (publicGlobalAlphaAction == nullptr)
        return;

    if (recursive) {
		actions().connectPrivateActionToPublicAction(&_globalAlphaToggle, &publicGlobalAlphaAction->getAlphaToggleAction(), recursive);
		actions().connectPrivateActionToPublicAction(&_globalAlphaValue, &publicGlobalAlphaAction->getAlphaIntegerAction(), recursive);
    }

    GroupAction::connectToPublicAction(publicAction, recursive);
}

void GlobalAlphaAction::disconnectFromPublicAction(bool recursive)
{
    if (!isConnected())
        return;

    if (recursive) {
        actions().disconnectPrivateActionFromPublicAction(&_globalAlphaToggle, recursive);
        actions().disconnectPrivateActionFromPublicAction(&_globalAlphaValue, recursive);
    }

    GroupAction::disconnectFromPublicAction(recursive);
}

void GlobalAlphaAction::fromVariantMap(const QVariantMap& variantMap)
{
    GroupAction::fromVariantMap(variantMap);

    _globalAlphaToggle.fromParentVariantMap(variantMap);
    _globalAlphaValue.fromParentVariantMap(variantMap);
}

QVariantMap GlobalAlphaAction::toVariantMap() const
{
    auto variantMap = GroupAction::toVariantMap();

    _globalAlphaToggle.insertIntoVariantMap(variantMap);
    _globalAlphaValue.insertIntoVariantMap(variantMap);

    return variantMap;
}

QWidget* GlobalAlphaAction::createWidget(QWidget* parent, int maxHeight)
{
    QWidget* widget = new QWidget(parent);
    QVBoxLayout* layout = new QVBoxLayout(widget);

    layout->addWidget(_globalAlphaToggle.createWidget(widget));
    layout->addWidget(_globalAlphaValue.createWidget(widget));

    widget->setLayout(layout);
    widget->setMaximumHeight(maxHeight);

    return widget;
}