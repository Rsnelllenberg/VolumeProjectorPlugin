#include "DVRViewPlugin.h"
#include "DVRWidget.h"
#include "VolumeRenderer.h"
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
    _spatialDataSet(),
    _valueDataSet(),
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
    _dropWidget->setDropIndicatorWidget(new DropWidget::DropIndicatorWidget(&getWidget(), "No data loaded", "Drag the DVRViewData from the data hierarchy here")); // TODO: bring this functionality back and fix it

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
            auto candidateDataset = mv::data().getDataset<Points>(datasetId);
            if (datasetId == getSpatialDataSetID()) {
                dropRegions << new DropWidget::DropRegion(this, "Warning", "Data already loaded", "exclamation-circle", false);
            } else {
                if(candidateDataset->getNumDimensions() == 3){
                    dropRegions << new DropWidget::DropRegion(this, "Points", QString("Set this dataset as spatial coordinates"), "map-marker-alt", true, [this, candidateDataset]() {
                        loadSpatialData({ candidateDataset });
                    });
                } else {
                    dropRegions << new DropWidget::DropRegion(this, "Warning", "Data is not 3 dimensions", "exclamation-circle", false);
                }

            }

            if (datasetId == getValueDataSetID()) {
                dropRegions << new DropWidget::DropRegion(this, "Warning", "Data already loaded", "exclamation-circle", false);
            } else {
                dropRegions << new DropWidget::DropRegion(this, "Points", QString("Set this dataset as item values"), "map-marker-alt", true, [this, candidateDataset]() {
                    loadValueData({ candidateDataset });
                    });
            }
        }
        else {
            dropRegions << new DropWidget::DropRegion(this, "Incompatible data", "This type of data is not supported", "exclamation-circle", false);
        }

        return dropRegions;
    });

    // update data when data set changed
    connect(&_valueDataSet, &Dataset<Points>::dataChanged, this, &DVRViewPlugin::renderData);
    connect(&_spatialDataSet, &Dataset<Points>::dataChanged, this, &DVRViewPlugin::renderData);

    // update settings UI when data set changed
    connect(&_valueDataSet, &Dataset<Points>::changed, this, [this]() {
        bool retFlag;
        updateUI(retFlag);
        if (retFlag) return;
    });

    connect(&_spatialDataSet, &Dataset<Points>::changed, this, [this]() {
        bool retFlag;
        updateUI(retFlag);
        if (retFlag) return;

        });

    // Create data so that we do not need to load any in this example
    createData();

    getLearningCenterAction().setPluginTitle("DVR OpenGL view");

    getLearningCenterAction().setShortDescription("DVR OpenGL view plugin");
    getLearningCenterAction().setLongDescription("This plugin shows how to implement a basic OpenGL-based view plugin in <b>ManiVault</b>.");


}

void DVRViewPlugin::updateUI(bool& retFlag)
{
    retFlag = true;
    const auto enabled = _valueDataSet.isValid();

    auto& nameString = _settingsAction.getDatasetNameAction();
    auto& pointSizeA = _settingsAction.getPointSizeAction();

    pointSizeA.setEnabled(enabled);

    if (!enabled)
        return;

    nameString.setString(_valueDataSet->getGuiName());
    retFlag = false;
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

    // Update the data when the scatter plot widget is initialized
    connect(_DVRWidget, &DVRWidget::initialized, this, []() { qDebug() << "DVRWidget is initialized."; } );

}

std::vector<int> DVRViewPlugin::getNumbersUpTo(int number) {
    std::vector<int> numbers;
    for (int i = 0; i <= number; ++i) {
        numbers.push_back(i);
    }
    return numbers;
}


void DVRViewPlugin::updateData()
{
    // Convert _spatialDataSet to std::vector<float> before passing to _DVRWidget->setData
    if (_valueDataSet.isValid() && _spatialDataSet.isValid()) {
        std::vector<float> spatialData;
        std::vector<float> valueData;
        qDebug() << "Convert Points datasets to floats";
        _spatialDataSet->populateDataForDimensions(spatialData, getNumbersUpTo(_spatialDataSet->getNumDimensions()), _spatialDataSet->indices);
        _valueDataSet->populateDataForDimensions(valueData, getNumbersUpTo(_valueDataSet->getNumDimensions()), _spatialDataSet->indices);

        _DVRWidget->setData(spatialData, valueData, _valueDataSet->getNumDimensions());
    }
}

void DVRViewPlugin::renderData()
{
    _DVRWidget->paintGL();
}

void DVRViewPlugin::loadValueData(const mv::Datasets& datasets)
{
    // Exit if there is nothing to load
    if (datasets.isEmpty())
        return;

    qDebug() << "DVRViewPlugin::loadValueData: Load data set from ManiVault core";
    _dropWidget->setShowDropIndicator(false);

    _valueDataSet = datasets.first();
    updateData();
    renderData();
}


void DVRViewPlugin::loadSpatialData(const mv::Datasets& datasets)
{
    // Exit if there is nothing to load
    if (datasets.isEmpty())
        return;

    qDebug() << "DVRViewPlugin::loadSpatialData: Load data set from ManiVault core";
    _dropWidget->setShowDropIndicator(false);

    _spatialDataSet = datasets.first();
    updateData();

    renderData();
}

QString DVRViewPlugin::getValueDataSetID() const
{
    if (_spatialDataSet.isValid())
        return _valueDataSet->getId();
    else
        return QString{};
}

QString DVRViewPlugin::getSpatialDataSetID() const
{
    if (_spatialDataSet.isValid())
        return _spatialDataSet->getId();
    else
        return QString{};
}

void DVRViewPlugin::createData()
{
    // Here, we create a random data set, so that we do not need 
    // to use other plugins for loading when trying out this example
    auto spatialPoints = mv::data().createDataset<Points>("Points", "ExampleDVRSpatialData");
    auto valuePoints = mv::data().createDataset<Points>("Points", "ExampleDVRValueData");

    int numPoints = 50;
    int numSpatialDimensions = 3;
    int numValueDimensions = 5;
    const std::vector<QString> dimSpatialNames {"x", "y", "z"};
    const std::vector<QString> dimValueNames{ "Dim 1", "Dim 2", "Dim 3", "Dim 4", "Dim 5"};

    qDebug() << "DVRViewPlugin::createData: Create some example data. " << numPoints << " points, each with " << numValueDimensions << " dimensions";

    // Create random example data
    std::vector<float> exampleSpatialData;
    std::vector<float> exampleValueData;
    {
        std::default_random_engine generator;
        std::uniform_real_distribution<float> distribution(0.0, 10.0);
        for (int i = 0; i < numPoints * numSpatialDimensions; i++)
        {
            exampleSpatialData.push_back(distribution(generator));
            //qDebug() << "exampleSpatialData[" << i << "]: " << exampleSpatialData[i];
        }
        for (int i = 0; i < numPoints * numValueDimensions; i++)
        {
            exampleValueData.push_back(distribution(generator));
            //qDebug() << "exampleValueData[" << i << "]: " << exampleValueData[i];
        }
    }

    // Passing example data
    spatialPoints->setData(exampleSpatialData.data(), numPoints, numSpatialDimensions);
    spatialPoints->setDimensionNames(dimSpatialNames);
    spatialPoints->setGuiName("DVRExampleSpatialDataset");

    valuePoints->setData(exampleValueData.data(), numPoints, numValueDimensions);
    valuePoints->setDimensionNames(dimValueNames);
    valuePoints->setGuiName("DVRExampleValueDataset");

    // Notify the core system of the new data
    events().notifyDatasetDataChanged(spatialPoints);
    events().notifyDatasetDataDimensionsChanged(spatialPoints);

    events().notifyDatasetDataChanged(valuePoints);
    events().notifyDatasetDataDimensionsChanged(valuePoints);
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
            for (auto& dataset : datasets) {
                if (getPluginInstance()->getSpatialDataSetID() == dataset.getDatasetId()) {
                    getPluginInstance()->loadSpatialData(Datasets({ dataset }));
                    qDebug() << "Plugin factory Spatial dimensions update";
                }
                if (getPluginInstance()->getValueDataSetID() == dataset.getDatasetId()) {
                    getPluginInstance()->loadValueData(Datasets({ dataset }));
                    qDebug() << "Plugin factory Value dimensions update";
                }
            }
        });

        pluginTriggerActions << pluginTriggerAction;
    }

    return pluginTriggerActions;
}
