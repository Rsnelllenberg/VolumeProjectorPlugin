#include "ExportAction.h"

#include "TransferFunctionPlugin.h"
#include "TransferFunctionWidget.h"

const QMap<ExportAction::Scale, TriggersAction::Trigger> ExportAction::triggers = QMap<ExportAction::Scale, TriggersAction::Trigger>({
    { ExportAction::Eighth, TriggersAction::Trigger("12.5%", "Scale by 1/8th") },
    { ExportAction::Quarter, TriggersAction::Trigger("25%", "Scale by a quarter") },
    { ExportAction::Half, TriggersAction::Trigger("50%", "Scale by half") },
    { ExportAction::One, TriggersAction::Trigger("100%", "Keep the original size") },
    { ExportAction::Twice, TriggersAction::Trigger("200%", "Scale twice") },
    { ExportAction::Thrice, TriggersAction::Trigger("300%", "Scale thrice") },
    { ExportAction::Four, TriggersAction::Trigger("400%", "Scale four times") },
    { ExportAction::Eight, TriggersAction::Trigger("800%", "Scale eight times") }
});

const QMap<ExportAction::Scale, float> ExportAction::scaleFactors = QMap<ExportAction::Scale, float>({
    { ExportAction::Eighth, 0.125f },
    { ExportAction::Quarter, 0.25f },
    { ExportAction::Half, 0.5f },
    { ExportAction::One, 1.0f },
    { ExportAction::Twice, 2.0f },
    { ExportAction::Thrice, 3.0f },
    { ExportAction::Four, 4.0f },
    { ExportAction::Eight, 8.0f }
});

ExportAction::ExportAction(QObject* parent, const QString& title) :
    VerticalGroupAction(parent, title),
    _transferFunctionPlugin(nullptr),
    _dimensionSelectionAction(this, "Dimensions"),
    _targetWidthAction(this, "Width ", 1, 10000),
    _targetHeightAction(this, "Height", 1, 10000),
    _lockAspectRatioAction(this, "Lock aspect ratio", true),
    _scaleAction(this, "Scale", triggers.values().toVector()),
    _backgroundColorAction(this, "Background color", QColor(Qt::white)),
    _overrideRangesAction(this, "Override ranges", false),
    _fixedRangeAction(this, "Fixed range"),
    _fileNamePrefixAction(this, "Filename prefix"),
    _statusAction(this, "Status"),
    _outputDirectoryAction(this, "Output"),
    _exportCancelAction(this, "Cancel", { TriggersAction::Trigger("Export", "Export dimensions"), TriggersAction::Trigger("Cancel", "Cancel export")  }),
    _aspectRatio()
{
    setIcon(mv::Application::getIconFont("FontAwesome").getIcon("camera"));
    setLabelWidthFixed(100);
    setConnectionPermissionsToForceNone(true);
    setConfigurationFlag(WidgetAction::ConfigurationFlag::ForceCollapsedInGroup);

    addAction(&_dimensionSelectionAction);
    addAction(&_targetWidthAction);
    addAction(&_targetHeightAction);
    addAction(&_lockAspectRatioAction);
    addAction(&_scaleAction);
    addAction(&_backgroundColorAction);
    addAction(&_overrideRangesAction);
    addAction(&_fixedRangeAction);
    addAction(&_outputDirectoryAction);
    addAction(&_fileNamePrefixAction);
    addAction(&_statusAction);
    addAction(&_exportCancelAction);

    _targetWidthAction.setEnabled(false);
    _targetHeightAction.setEnabled(false);
    _lockAspectRatioAction.setEnabled(false);
    _scaleAction.setEnabled(false);

    _targetWidthAction.setSuffix("px");
    _targetHeightAction.setSuffix("px");
}

void ExportAction::initialize(TransferFunctionPlugin* transferFunctionPlugin)
{
    Q_ASSERT(transferFunctionPlugin != nullptr);

    if (transferFunctionPlugin == nullptr)
        return;

    _transferFunctionPlugin = transferFunctionPlugin;

    _outputDirectoryAction.setSettingsPrefix(_transferFunctionPlugin, "Screenshot/OutputDirectory");

    connect(&_transferFunctionPlugin->getPositionDataset(), &Dataset<Points>::changed, this, &ExportAction::updateDimensionsPickerAction);

    const auto scale = [this](float scaleFactor) {
        _targetWidthAction.setValue(scaleFactor * static_cast<float>(_transferFunctionPlugin->getTransferFunctionWidget().width()));
        _targetHeightAction.setValue(scaleFactor * static_cast<float>(_transferFunctionPlugin->getTransferFunctionWidget().height()));
    };

    connect(&_scaleAction, &TriggersAction::triggered, this, [this, scale](std::int32_t triggerIndex) {
        scale(scaleFactors.values().at(triggerIndex));
    });

    const auto positionDatasetChanged = [this]() -> void {
        auto& positionDataset = _transferFunctionPlugin->getPositionDataset();

        if (!positionDataset.isValid())
            return;

        _dimensionSelectionAction.setObjectName("Dimensions/" + positionDataset->text());
        _fileNamePrefixAction.setString(positionDataset->text() + "_");
    };

    connect(&_transferFunctionPlugin->getPositionDataset(), &Dataset<DatasetImpl>::changed, this, positionDatasetChanged);

    const auto updateTargetHeightAction = [this]() -> void {
        _targetHeightAction.setEnabled(!_lockAspectRatioAction.isChecked());
    };

    const auto updateAspectRatio = [this]() -> void {
        _aspectRatio = static_cast<float>(_targetHeightAction.getValue()) / static_cast<float>(_targetWidthAction.getValue());
    };

    connect(&_lockAspectRatioAction, &ToggleAction::toggled, this, updateTargetHeightAction);
    connect(&_lockAspectRatioAction, &ToggleAction::toggled, this, updateAspectRatio);

    connect(&_targetWidthAction, &IntegralAction::valueChanged, this, [this]() {
        if (_lockAspectRatioAction.isChecked())
            _targetHeightAction.setValue(static_cast<std::int32_t>(_aspectRatio * static_cast<float>(_targetWidthAction.getValue())));
    });

    connect(&_exportCancelAction, &TriggersAction::triggered, this, [this](std::int32_t triggerIndex) {
        switch (triggerIndex)
        {
            case 0:
                exportImages();
                break;

            case 1:
                break;

            default:
                break;
        }
    });

    const auto updateFixedRangeReadOnly = [this]() {
        _fixedRangeAction.setEnabled(_overrideRangesAction.isChecked());
    };

    connect(&_overrideRangesAction, &ToggleAction::toggled, this, updateFixedRangeReadOnly);

    connect(&_fileNamePrefixAction, &StringAction::stringChanged, this, &ExportAction::updateExportTrigger);
    connect(&_outputDirectoryAction, &DirectoryPickerAction::directoryChanged, this, &ExportAction::updateExportTrigger);

    updateAspectRatio();
    updateTargetHeightAction();
    updateFixedRangeReadOnly();

    initializeTargetSize();
    updateDimensionsPickerAction();

    updateExportTrigger();
}

void ExportAction::initializeTargetSize()
{
    const auto scatterPlotWidgetSize = _transferFunctionPlugin->getTransferFunctionWidget().size();

    _targetWidthAction.initialize(1, 8 * scatterPlotWidgetSize.width(), scatterPlotWidgetSize.width());
    _targetHeightAction.initialize(1, 8 * scatterPlotWidgetSize.height(), scatterPlotWidgetSize.height());

    _aspectRatio = static_cast<float>(_targetHeightAction.getValue()) / static_cast<float>(_targetWidthAction.getValue());
}

void ExportAction::exportImages()
{
   
}

void ExportAction::updateDimensionsPickerAction()
{
    _dimensionSelectionAction.setPointsDataset(_transferFunctionPlugin->getPositionDataset());

    connect(&_dimensionSelectionAction.getItemModel(), &QAbstractItemModel::dataChanged, this, &ExportAction::updateExportTrigger);
    connect(&_dimensionSelectionAction.getProxyModel(), &QAbstractItemModel::modelReset, this, &ExportAction::updateExportTrigger);

    updateExportTrigger();
}

bool ExportAction::mayExport() const
{
    if (_fileNamePrefixAction.getString().isEmpty())
        return false;

    if (!_outputDirectoryAction.isValid())
        return false;

    if (getNumberOfSelectedDimensions() == 0)
        return false;

    return true;
}

std::int32_t ExportAction::getNumberOfSelectedDimensions() const
{
    const auto enabledDimensions = _dimensionSelectionAction.getEnabledDimensions();

    return std::count_if(enabledDimensions.begin(), enabledDimensions.end(), [](bool value) {
        return value;
    });
}

void ExportAction::updateExportTrigger()
{
    _exportCancelAction.setTriggerText(0, getNumberOfSelectedDimensions() == 0 ? "Nothing to export" : "Export (" + QString::number(getNumberOfSelectedDimensions()) + ")");
    _exportCancelAction.setTriggerTooltip(0, getNumberOfSelectedDimensions() == 0 ? "There are no images selected to export" : "Export " + QString::number(getNumberOfSelectedDimensions()) + " image" + (getNumberOfSelectedDimensions() >= 2 ? "s" : "") + " to disk");
    _exportCancelAction.setTriggerEnabled(0, mayExport());
}

void ExportAction::fromVariantMap(const QVariantMap& variantMap)
{
  
}

QVariantMap ExportAction::toVariantMap() const
{
    auto variantMap = GroupAction::toVariantMap();

   
    return variantMap;
}
