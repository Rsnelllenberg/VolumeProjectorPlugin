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
    _currentDataSet(),
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
        const auto dataTypes = DataTypes({ PointType });

        if (dataTypes.contains(dataType)) {

            if (datasetId == getCurrentDataSetID()) {
                dropRegions << new DropWidget::DropRegion(this, "Warning", "Data already loaded", "exclamation-circle", false);
            }
            else {
                auto candidateDataset = mv::data().getDataset<Points>(datasetId);

                dropRegions << new DropWidget::DropRegion(this, "Points", QString("Visualize %1 as parallel coordinates").arg(datasetGuiName), "map-marker-alt", true, [this, candidateDataset]() {
                    loadData({ candidateDataset });
                    });

            }
        }
        else {
            dropRegions << new DropWidget::DropRegion(this, "Incompatible data", "This type of data is not supported", "exclamation-circle", false);
        }

        return dropRegions;
    });

    // update data when data set changed
    connect(&_currentDataSet, &Dataset<Points>::dataChanged, this, &DVRViewPlugin::updatePlot);

    // update settings UI when data set changed
    connect(&_currentDataSet, &Dataset<Points>::changed, this, [this]() {
        const auto enabled = _currentDataSet.isValid();

        auto& nameString = _settingsAction.getDatasetNameAction();
        auto& xDimPicker = _settingsAction.getXDimensionPickerAction();
        auto& yDimPicker = _settingsAction.getYDimensionPickerAction();
        auto& pointSizeA = _settingsAction.getPointSizeAction();

        xDimPicker.setEnabled(enabled);
        yDimPicker.setEnabled(enabled);
        pointSizeA.setEnabled(enabled);

        if (!enabled)
            return;

        nameString.setString(_currentDataSet->getGuiName());

        xDimPicker.setPointsDataset(_currentDataSet);
        yDimPicker.setPointsDataset(_currentDataSet);

        xDimPicker.setCurrentDimensionIndex(0);

        const auto yIndex = xDimPicker.getNumberOfDimensions() >= 2 ? 1 : 0;
        yDimPicker.setCurrentDimensionIndex(yIndex);

    });

    // Create data so that we do not need to load any in this example
    createData();

    getLearningCenterAction().setPluginTitle("DVR OpenGL view");

    getLearningCenterAction().setShortDescription("DVR OpenGL view plugin");
    getLearningCenterAction().setLongDescription("This plugin shows how to implement a basic OpenGL-based view plugin in <b>ManiVault</b>.");

    getLearningCenterAction().addVideos(QStringList({ "Practitioner", "Developer" }));
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
    //if (!_currentDataSet.isValid())
    //{
    //    qDebug() << "DVRViewPlugin:: dataset is not valid - no data will be displayed";
    //    return;
    //}

    //if (_currentDataSet->getNumDimensions() < 2)
    //{
    //    qDebug() << "DVRViewPlugin:: dataset must have at least two dimensions";
    //    return;
    //}

    //// Retrieve the data that is to be shown from the core
    //auto newDimX = _settingsAction.getXDimensionPickerAction().getCurrentDimensionIndex();
    //auto newDimY = _settingsAction.getYDimensionPickerAction().getCurrentDimensionIndex();

    //if (newDimX >= 0)
    //    _currentDimensions[0] = static_cast<unsigned int>(newDimX);

    //if (newDimY >= 0)
    //    _currentDimensions[1] = static_cast<unsigned int>(newDimY);

    //std::vector<mv::Vector3f> spatialdata;
    //_currentDataSet->populateDataForDimensions(spatialdata, std::vector{0,1,2});

    //std::vector<std::vector<float>> valueData;
    //std::vector<int> dimensions;
    //for (int i = 3; i < _currentDataSet->getNumDimensions(); ++i) {
    //    dimensions.push_back(i);
    //}
    //_currentDataSet->populateDataForDimensions(valueData, dimensions);

    //// Set data in OpenGL widget
    //_DVRWidget->setData(spatialdata, valueData);
}


void DVRViewPlugin::loadData(const mv::Datasets& datasets)
{
    // Exit if there is nothing to load
    if (datasets.isEmpty())
        return;

    qDebug() << "DVRViewPlugin::loadData: Load data set from ManiVault core";
    _dropWidget->setShowDropIndicator(false);

    // Load the first dataset, changes to _currentDataSet are connected with convertDataAndUpdateChart
    _currentDataSet = datasets.first();
    updatePlot();
}

QString DVRViewPlugin::getCurrentDataSetID() const
{
    if (_currentDataSet.isValid())
        return _currentDataSet->getId();
    else
        return QString{};
}

void DVRViewPlugin::createData()
{
    // Here, we create a random data set, so that we do not need 
    // to use other plugins for loading when trying out this example
    auto points = mv::data().createDataset<Points>("Points", "DVRViewData");

    int numPoints = 1000;
    const std::vector<QString> dimNames{ "Dim x", "Dim y", "Dim z", "Dim v1", "Dim v2", "Dim v3", "Dim 4", "Dim v5"};
    int numDimensions = dimNames.size();

    qDebug() << "DVRViewPlugin::createData: Create some example data. " << numPoints << " points, each with " << numDimensions << " dimensions";

    // Create random example data
    std::vector<float> exampleData;
    {
        std::default_random_engine generator;
        std::uniform_real_distribution<float> distribution(0.0, 10.0);

        for (int i = 0; i < numPoints * numDimensions; i++)
        {
            exampleData.push_back(distribution(generator));
            //qDebug() << "exampleData[" << i << "]: " << exampleData[i];
        }
    }

    // Passing example data with 1000 points and 2 dimensions
    points->setData(exampleData.data(), numPoints, numDimensions);
    points->setDimensionNames(dimNames);

    // Notify the core system of the new data
    events().notifyDatasetDataChanged(points);
    events().notifyDatasetDataDimensionsChanged(points);
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
                getPluginInstance()->loadData(Datasets({ dataset }));
        });

        pluginTriggerActions << pluginTriggerAction;
    }

    return pluginTriggerActions;
}
