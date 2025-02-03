#include "MiscellaneousAction.h"

#include "TransferFunctionPlugin.h"
#include "TransferFunctionWidget.h"

using namespace mv::gui;

const QColor MiscellaneousAction::DEFAULT_BACKGROUND_COLOR = qRgb(255, 255, 255);

MiscellaneousAction::MiscellaneousAction(QObject* parent, const QString& title) :
    VerticalGroupAction(parent, title),
    _transferFunctionPlugin(dynamic_cast<TransferFunctionPlugin*>(parent->parent())),
    _backgroundColorAction(this, "Background color"),
    _randomizedDepthAction(this, "Randomized depth", true)
{
    setIcon(Application::getIconFont("FontAwesome").getIcon("cog"));
    setLabelSizingType(LabelSizingType::Auto);
    setConfigurationFlag(WidgetAction::ConfigurationFlag::ForceCollapsedInGroup);

    addAction(&_backgroundColorAction);
    addAction(&_randomizedDepthAction);

    _backgroundColorAction.setColor(DEFAULT_BACKGROUND_COLOR);

    const auto updateBackgroundColor = [this]() -> void {
        _transferFunctionPlugin->getTransferFunctionWidget().setBackgroundColor(_backgroundColorAction.getColor());
    };

    connect(&_backgroundColorAction, &ColorAction::colorChanged, this, [this, updateBackgroundColor](const QColor& color) {
        updateBackgroundColor();
    });

    updateBackgroundColor();

    const auto updateRandomizedDepth = [this]() -> void {
        _transferFunctionPlugin->getTransferFunctionWidget().setRandomizedDepthEnabled(_randomizedDepthAction.isChecked());
    };

    connect(&_randomizedDepthAction, &ToggleAction::toggled, this, [this, updateRandomizedDepth](bool toggled) {
        updateRandomizedDepth();
    });

    updateRandomizedDepth();
}

QMenu* MiscellaneousAction::getContextMenu()
{
    auto menu = new QMenu("Miscellaneous");

    menu->addAction(&_backgroundColorAction);

    return menu;
}

void MiscellaneousAction::connectToPublicAction(WidgetAction* publicAction, bool recursive)
{
    auto publicMiscellaneousAction = dynamic_cast<MiscellaneousAction*>(publicAction);

    Q_ASSERT(publicMiscellaneousAction != nullptr);

    if (publicMiscellaneousAction == nullptr)
        return;

    if (recursive) {
        actions().connectPrivateActionToPublicAction(&_backgroundColorAction, &publicMiscellaneousAction->getBackgroundColorAction(), recursive);
    }

    GroupAction::connectToPublicAction(publicAction, recursive);
}

void MiscellaneousAction::disconnectFromPublicAction(bool recursive)
{
    if (!isConnected())
        return;

    if (recursive) {
        actions().disconnectPrivateActionFromPublicAction(&_backgroundColorAction, recursive);
    }

    GroupAction::disconnectFromPublicAction(recursive);
}

void MiscellaneousAction::fromVariantMap(const QVariantMap& variantMap)
{
    GroupAction::fromVariantMap(variantMap);

    _backgroundColorAction.fromParentVariantMap(variantMap);
    _randomizedDepthAction.fromParentVariantMap(variantMap);
}

QVariantMap MiscellaneousAction::toVariantMap() const
{
    auto variantMap = GroupAction::toVariantMap();

    _backgroundColorAction.insertIntoVariantMap(variantMap);
    _randomizedDepthAction.insertIntoVariantMap(variantMap);

    return variantMap;
}