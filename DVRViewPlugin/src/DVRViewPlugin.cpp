#include "DVRViewPlugin.h"
#include "DVRWidget.h"

#include "GlobalSettingsAction.h"

#include <graphics/Vector2f.h>

#include <DatasetsMimeData.h>

#include <QLabel>
#include <QDebug>

#include <random>
#include <numeric>

Q_PLUGIN_METADATA(IID "studio.manivault.DVRViewPlugin")

using namespace mv;

// -----------------------------------------------------------------------------
// DVRViewPlugin
// -----------------------------------------------------------------------------
DVRViewPlugin::DVRViewPlugin(const PluginFactory* factory) :
    ViewPlugin(factory),
    _volumeDataset(),
    _currentDimensions({0, 1}),
    _dropWidget(nullptr),
    _DVRWidget(new DVRWidget()),
    _settingsAction(this, "Settings Action")
{
    setObjectName("DVR OpenGL view");

    // Instantiate new drop widget, setting the DVR Widget as its parent
    // the parent widget hat to setAcceptDrops(true) for the drop widget to work
    _dropWidget = new DropWidget(_DVRWidget);

    // Set the drop indicator widget (the widget that indicates that the view is eligible for data dropping)
    _dropWidget->setDropIndicatorWidget(new DropWidget::DropIndicatorWidget(&getWidget(), "No data loaded", "Drag the DVRViewData from the data hierarchy here"));

    // Initialize the drop regions
    _dropWidget->initialize([this](const QMimeData* mimeData) -> DropWidget::DropRegions {
        // A drop widget can contain zero or more drop regions
        DropWidget::DropRegions dropRegions;

        const auto datasetsMimeData = dynamic_cast<const DatasetsMimeData*>(mimeData);

        if (datasetsMimeData == nullptr)
            return dropRegions;

        if (datasetsMimeData->getDatasets().count() > 1)
            return dropRegions;

        // Gather information to generate appropriate drop regions
        const auto dataset = datasetsMimeData->getDatasets().first();
        const auto datasetGuiName = dataset->getGuiName();
        const auto datasetId = dataset->getId();
        const auto dataType = dataset->getDataType();
        const auto dataTypes = DataTypes({ VolumeType });

        if (dataTypes.contains(dataType)) {

            if (datasetId == getCurrentDataSetID()) {
                dropRegions << new DropWidget::DropRegion(this, "Warning", "Data already loaded", "exclamation-circle", false);
            }
            else {
                auto candidateDataset = mv::data().getDataset<Volumes>(datasetId);

                dropRegions << new DropWidget::DropRegion(this, "Volumes", QString("Visualize %1 as parallel coordinates").arg(datasetGuiName), "map-marker-alt", true, [this, candidateDataset]() {
                    loadData({ candidateDataset });
                    });

            }
        }
        else {
            dropRegions << new DropWidget::DropRegion(this, "Incompatible data", "This type of data is not supported it only supports Volume Data", "exclamation-circle", false);
        }

        return dropRegions;
    });

    // update data when data set changed
    connect(&_volumeDataset, &Dataset<Points>::dataChanged, this, &DVRViewPlugin::updateData);

    // update settings UI when data set changed
    connect(&_volumeDataset, &Dataset<Points>::changed, this, [this]() {
        const auto enabled = _volumeDataset.isValid();

        auto& nameString = _settingsAction.getDatasetNameAction();
        auto& xDimPicker = _settingsAction.getXDimensionPickerAction();
        auto& yDimPicker = _settingsAction.getYDimensionPickerAction();
        auto& zDimPicker = _settingsAction.getZDimensionPickerAction();

        xDimPicker.setEnabled(enabled);
        yDimPicker.setEnabled(enabled);
        zDimPicker.setEnabled(enabled);

        if (!enabled)
            return;

        nameString.setString(_volumeDataset->getGuiName());
       

    });
}

void DVRViewPlugin::init()
{
    // Create layout
    auto layout = new QVBoxLayout();

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(_DVRWidget, 100);

    // Apply the layout
    getWidget().setLayout(layout);

    addDockingAction(&_settingsAction);
    _DVRWidget->installEventFilter(this);
    // Update the data when the scatter plot widget is initialized
    connect(_DVRWidget, &DVRWidget::initialized, this, []() { qDebug() << "DVRWidget is initialized."; } );

}

void DVRViewPlugin::updatePlot()
{
    _DVRWidget->setClippingPlaneBoundery(_settingsAction.getXDimensionPickerAction().getRange().getMinimum(),
        _settingsAction.getXDimensionPickerAction().getRange().getMaximum(),
        _settingsAction.getYDimensionPickerAction().getRange().getMinimum(),
        _settingsAction.getYDimensionPickerAction().getRange().getMaximum(),
        _settingsAction.getZDimensionPickerAction().getRange().getMinimum(),
        _settingsAction.getZDimensionPickerAction().getRange().getMaximum());
    _DVRWidget->update();
}

void DVRViewPlugin::updateData()
{
    if (_volumeDataset.isValid()) {
        std::vector<std::uint32_t> dimensionIndices = generateSequence(std::min(8, int(_volumeDataset->getComponentsPerVoxel()))); // TODO remove the max 8 componest part later just there for now to avoid memory crashes
        _DVRWidget->setData(_volumeDataset, dimensionIndices);
    }
    else {
        qDebug() << "DVRViewPlugin::updateData: No data to update";
    }
}


void DVRViewPlugin::loadData(const mv::Dataset<Points>& dataset)
{
    qDebug() << "DVRViewPlugin::loadData: Load data set from ManiVault core";
    _dropWidget->setShowDropIndicator(false);

    _volumeDataset = dataset;
    updateData();
}

QString DVRViewPlugin::getCurrentDataSetID() const
{
    if (_volumeDataset.isValid())
        return _volumeDataset->getId();
    else
        return QString{};
}


std::vector<std::uint32_t> DVRViewPlugin::generateSequence(int n) {
    std::vector<std::uint32_t> sequence(n);
    std::iota(sequence.begin(), sequence.end(), 0);
    return sequence;
}

// -----------------------------------------------------------------------------
// DVRViewPluginFactory
// -----------------------------------------------------------------------------

ViewPlugin* DVRViewPluginFactory::produce()
{
    return new DVRViewPlugin(this);
}

DVRViewPluginFactory::DVRViewPluginFactory() :
    ViewPluginFactory(),
    _statusBarAction(nullptr),
    _statusBarPopupGroupAction(this, "Popup Group"),
    _statusBarPopupAction(this, "Popup")
{
    
}

void DVRViewPluginFactory::initialize()
{
    ViewPluginFactory::initialize();

    // Create an instance of our GlobalSettingsAction (derived from PluginGlobalSettingsGroupAction) and assign it to the factory
    setGlobalSettingsGroupAction(new GlobalSettingsAction(this, this));

    // Configure the status bar popup action
    _statusBarPopupAction.setDefaultWidgetFlags(StringAction::Label);
    _statusBarPopupAction.setString("<p><b>DVR OpenGL View</b></p><p>This is an example of a plugin status bar item</p><p>A concrete example on how this status bar was created can be found <a href='https://github.com/ManiVaultStudio/ExamplePlugins/blob/master/ExampleViewOpenGL/src/DVRViewPlugin.cpp'>here</a>.</p>");
    _statusBarPopupAction.setPopupSizeHint(QSize(200, 10));

    _statusBarPopupGroupAction.setShowLabels(false);
    _statusBarPopupGroupAction.setConfigurationFlag(WidgetAction::ConfigurationFlag::NoGroupBoxInPopupLayout);
    _statusBarPopupGroupAction.addAction(&_statusBarPopupAction);
    _statusBarPopupGroupAction.setWidgetConfigurationFunction([](WidgetAction* action, QWidget* widget) -> void {
        auto label = widget->findChild<QLabel*>("Label");

        Q_ASSERT(label != nullptr);

        if (label == nullptr)
            return;

        label->setOpenExternalLinks(true);
    });

    _statusBarAction = new PluginStatusBarAction(this, "DVR View OpenGL", getKind());

    // Sets the action that is shown when the status bar is clicked
    _statusBarAction->setPopupAction(&_statusBarPopupGroupAction);

    // Position to the right of the status bar action
    _statusBarAction->setIndex(-1);

    // Assign the status bar action so that it will appear on the main window status bar
    setStatusBarAction(_statusBarAction);
}

QIcon DVRViewPluginFactory::getIcon(const QColor& color /*= Qt::black*/) const
{
    return mv::Application::getIconFont("FontAwesome").getIcon("braille", color);
}

mv::DataTypes DVRViewPluginFactory::supportedDataTypes() const
{
    DataTypes supportedTypes;

    // This example analysis plugin is compatible with points datasets
    supportedTypes.append(PointType);

    return supportedTypes;
}

mv::gui::PluginTriggerActions DVRViewPluginFactory::getPluginTriggerActions(const mv::Datasets& datasets) const
{
    PluginTriggerActions pluginTriggerActions;

    const auto getPluginInstance = [this]() -> DVRViewPlugin* {
        return dynamic_cast<DVRViewPlugin*>(plugins().requestViewPlugin(getKind()));
    };

    const auto numberOfDatasets = datasets.count();

    if (numberOfDatasets >= 1 && PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
        auto pluginTriggerAction = new PluginTriggerAction(const_cast<DVRViewPluginFactory*>(this), this, "Example GL", "OpenGL view example data", getIcon(), [this, getPluginInstance, datasets](PluginTriggerAction& pluginTriggerAction) -> void {
            for (auto& dataset : datasets)
                getPluginInstance()->loadData(dataset);
        });

        pluginTriggerActions << pluginTriggerAction;
    }

    return pluginTriggerActions;
}
