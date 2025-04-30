#include "DVRViewPlugin.h"
#include "DVRWidget.h"

#include "GlobalSettingsAction.h"

#include <graphics/Vector2f.h>

#include <DatasetsMimeData.h>

#include <QLabel>
#include <QDebug>

#include <random>
#include <numeric>
#include <hnswlib.h>
#include <iostream>
#include <vector>

Q_PLUGIN_METADATA(IID "studio.manivault.DVRViewPlugin")

using namespace mv;

// -----------------------------------------------------------------------------
// DVRViewPlugin
// -----------------------------------------------------------------------------
DVRViewPlugin::DVRViewPlugin(const PluginFactory* factory) :
    ViewPlugin(factory),
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
        const auto dataTypes = DataTypes({ VolumeType, ImageType, PointType });

        if (dataTypes.contains(dataType)) {
            if (dataType == VolumeType) {
                if (datasetId == getVolumeDataSetID()) {
                    dropRegions << new DropWidget::DropRegion(this, "Warning", "Data already loaded", "exclamation-circle", false);
                }
                else {
                    auto candidateDataset = mv::data().getDataset<Volumes>(datasetId);

                    dropRegions << new DropWidget::DropRegion(this, "Volumes", QString("Visualize %1 as a Volume").arg(datasetGuiName), "map-marker-alt", true, [this, candidateDataset]() {
                        loadData({ candidateDataset });
                        });

                }
            }
            else if (dataType == ImageType) {
                if (datasetId == getTfDatasetID()) {
                    dropRegions << new DropWidget::DropRegion(this, "Warning", "Data already loaded", "exclamation-circle", false);
                }
                else {
                    auto candidateDataset = mv::data().getDataset<Images>(datasetId);
                    dropRegions << new DropWidget::DropRegion(this, "Images", QString("Pass %1 along as the transfer function").arg(datasetGuiName), "map-marker-alt", true, [this, candidateDataset]() {
                        loadTfData(candidateDataset);
                        });
                }

                if (datasetId == getMaterialTransitionDataSetID()) {
                    dropRegions << new DropWidget::DropRegion(this, "Warning", "Data already loaded", "exclamation-circle", false);
                }
                else {
                    auto candidateDataset = mv::data().getDataset<Images>(datasetId);
                    dropRegions << new DropWidget::DropRegion(this, "Images", QString("Pass %1 along as the material transition texture").arg(datasetGuiName), "map-marker-alt", true, [this, candidateDataset]() {
                        loadMaterialTransitionData(candidateDataset);
                        });
                }

                if (datasetId == getMaterialPositionsDataSetID()) {
                    dropRegions << new DropWidget::DropRegion(this, "Warning", "Data already loaded", "exclamation-circle", false);
                }
                else {
                    auto candidateDataset = mv::data().getDataset<Images>(datasetId);
                    dropRegions << new DropWidget::DropRegion(this, "Images", QString("Pass %1 along as the material positions texture").arg(datasetGuiName), "map-marker-alt", true, [this, candidateDataset]() {
                        loadMaterialPositionsData(candidateDataset);
                        });
                }
            }
            else if (dataType == PointType) {
                if (datasetId == getReducedPosDataSetID()) {
                    dropRegions << new DropWidget::DropRegion(this, "Warning", "Data already loaded", "exclamation-circle", false);
                }
                else {
                    auto candidateDataset = mv::data().getDataset<Points>(datasetId);
                    if (candidateDataset->getNumDimensions() == 2) {
                        dropRegions << new DropWidget::DropRegion(this, "Points", QString("Pass %1 along as dimension reduced point locations").arg(datasetGuiName), "map-marker-alt", true, [this, candidateDataset]() {
                            loadReducedPosData({ candidateDataset });
                            });
                    }
                    else {
                        dropRegions << new DropWidget::DropRegion(this, "Incompatible data", "This type of data is not supported it only supports 2D data", "exclamation-circle", false);
                    }
                }
            }
        }
        else {
            dropRegions << new DropWidget::DropRegion(this, "Incompatible data", "This type of data is not supported it only supports Volume Data", "exclamation-circle", false);
        }

        return dropRegions;
    });

    // update data when data set changed
    connect(&_volumeDataset, &Dataset<Points>::dataChanged, this, &DVRViewPlugin::updateVolumeData);
    connect(&_tfTexture, &Dataset<Images>::dataChanged, this, &DVRViewPlugin::updateTfData);
    connect(&_reducedPosDataset, &Dataset<Points>::dataChanged, this, &DVRViewPlugin::updateReducedPosData);
    connect(&_materialTransitionTexture, &Dataset<Images>::dataChanged, this, &DVRViewPlugin::updateMaterialTransitionData);
    connect(&_materialPositionTexture, &Dataset<Images>::dataChanged, this, &DVRViewPlugin::updateMaterialPositionsData); // New connection

    // update settings UI when data set changed
    connect(&_volumeDataset, &Dataset<Points>::changed, this, [this]() {
        const auto enabled = _volumeDataset.isValid();

        auto& nameString = _settingsAction.getDatasetNameAction();
        auto& renderMode = _settingsAction.getRenderModeAction();
        auto& mipDimension = _settingsAction.getMIPDimensionPickerAction();

        auto& xDimPicker = _settingsAction.getXDimClippingPlaneAction();
        auto& yDimPicker = _settingsAction.getYDimClippingPlaneAction();
        auto& zDimPicker = _settingsAction.getZDimClippingPlaneAction();

        auto& usecustomRenderSpace = _settingsAction.getUseCustomRenderSpaceAction(); 

        auto& xRenderSize = _settingsAction.getXRenderSizeAction();
        auto& yRenderSize = _settingsAction.getYRenderSizeAction();
        auto& zRenderSize = _settingsAction.getZRenderSizeAction();
        renderMode.setEnabled(enabled);

        xDimPicker.setEnabled(enabled);
        yDimPicker.setEnabled(enabled);
        zDimPicker.setEnabled(enabled);

        usecustomRenderSpace.setEnabled(enabled);

        xRenderSize.setEnabled(enabled);
        yRenderSize.setEnabled(enabled);
        zRenderSize.setEnabled(enabled);

        if (!enabled)
            return;

        nameString.setString(_volumeDataset->getGuiName());
        auto parent = _volumeDataset->getParent();

        if (parent->getDataType() == PointType) {
            auto points = mv::Dataset<Points>(parent);
            mipDimension.setPointsDataset(points);
        }
        else {
            qCritical() << "DVRViewPlugin::updateSettings: Parent data set is not a point data set";
        }

       

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

void DVRViewPlugin::updateRenderSettings()
{
    if (_settingsAction.getRenderModeAction().getCurrentText() == "1D MIP") {
        _settingsAction.getMIPDimensionPickerAction().setEnabled(true);
    }
    else {
        _settingsAction.getMIPDimensionPickerAction().setEnabled(false);
    }

    if (_settingsAction.getUseCustomRenderSpaceAction().isChecked()) {
        _settingsAction.getXRenderSizeAction().setEnabled(true);
        _settingsAction.getYRenderSizeAction().setEnabled(true);
        _settingsAction.getZRenderSizeAction().setEnabled(true);
    }
    else {
        _settingsAction.getXRenderSizeAction().setEnabled(false);
        _settingsAction.getYRenderSizeAction().setEnabled(false);
        _settingsAction.getZRenderSizeAction().setEnabled(false);
    }

    _DVRWidget->setClippingPlaneBoundery(_settingsAction.getXDimClippingPlaneAction().getRange().getMinimum(),
        _settingsAction.getXDimClippingPlaneAction().getRange().getMaximum(),
        _settingsAction.getYDimClippingPlaneAction().getRange().getMinimum(),
        _settingsAction.getYDimClippingPlaneAction().getRange().getMaximum(),
        _settingsAction.getZDimClippingPlaneAction().getRange().getMinimum(),
        _settingsAction.getZDimClippingPlaneAction().getRange().getMaximum());

    _DVRWidget->setUseCustomRenderSpace(_settingsAction.getUseCustomRenderSpaceAction().isChecked());
    _DVRWidget->setRenderSpace(_settingsAction.getXRenderSizeAction().getValue(),
        _settingsAction.getYRenderSizeAction().getValue(),
        _settingsAction.getZRenderSizeAction().getValue());

    _DVRWidget->setStepSize(_settingsAction.getStepSizeAction().getValue());
    _DVRWidget->setRenderMode(_settingsAction.getRenderModeAction().getCurrentText());
    _DVRWidget->setMIPDimension(_settingsAction.getMIPDimensionPickerAction().getCurrentDimensionIndex());
    _DVRWidget->setUseShading(_settingsAction.getUseShaderAction().isChecked());
    _DVRWidget->setUseEmptySpaceSkipping(_settingsAction.getUseEmptySpaceSkippingAction().isChecked());
    _DVRWidget->setRenderCubeSize(_settingsAction.getRenderCubeSizeAction().getValue());

    _DVRWidget->update();
}

void DVRViewPlugin::updateVolumeData()
{
    if (_volumeDataset.isValid()) {
        std::vector<std::uint32_t> dimensionIndices = generateSequence(_volumeDataset->getComponentsPerVoxel()); // TODO remove the max 8 componest part later just there for now to avoid memory crashes
        _DVRWidget->setData(_volumeDataset, dimensionIndices);
    }
    else {
        qDebug() << "DVRViewPlugin::updateVolumeData: No data to update";
    }
}

void DVRViewPlugin::updateTfData()
{
    if (_tfTexture.isValid()) {
        _DVRWidget->setTfTexture(_tfTexture);
    }
    else {
        qDebug() << "DVRViewPlugin::updateTfData: No data to update";
    }
}

void DVRViewPlugin::updateReducedPosData()
{
    if (_reducedPosDataset.isValid()) {
        _DVRWidget->setReducedPosData(_reducedPosDataset);
    }
    else {
        qDebug() << "DVRViewPlugin::updateReducedPosData: No data to update";
    }
}

void DVRViewPlugin::updateMaterialTransitionData()
{
    if (_materialTransitionTexture.isValid()) {
        _DVRWidget->setMaterialTransitionTexture(_materialTransitionTexture);
    }
    else {
        qDebug() << "DVRViewPlugin::updateMaterialTransitionData: No data to update";
    }
}

void DVRViewPlugin::updateMaterialPositionsData()
{
    if (_materialPositionTexture.isValid()) {
        _DVRWidget->setMaterialPositionTexture(_materialPositionTexture);
    }
    else {
        qDebug() << "DVRViewPlugin::updateMaterialPositionsData: No data to update";
    }
}


void DVRViewPlugin::loadData(const mv::Dataset<Points>& dataset)
{
    _volumeDataset = dataset;
    updateShowDropIndicator();
    updateVolumeData();
}

void DVRViewPlugin::loadTfData(const mv::Dataset<Images>& dataset)
{
    _tfTexture = dataset;
    updateShowDropIndicator();
    updateTfData();
}

void DVRViewPlugin::loadReducedPosData(const mv::Dataset<Points>& dataset)
{
    _reducedPosDataset = dataset;
    updateShowDropIndicator();
    updateReducedPosData();
}

void DVRViewPlugin::loadMaterialTransitionData(const mv::Dataset<Images>& datasets)
{
    _materialTransitionTexture = datasets;
    updateShowDropIndicator();
    updateMaterialTransitionData();
}

void DVRViewPlugin::loadMaterialPositionsData(const mv::Dataset<Images>& datasets)
{
    _materialPositionTexture = datasets;
    updateShowDropIndicator();
    updateMaterialPositionsData();
}

void DVRViewPlugin::updateShowDropIndicator()
{
    if (_tfTexture.isValid() && _volumeDataset.isValid() && _reducedPosDataset.isValid() && _materialTransitionTexture.isValid() && _materialPositionTexture.isValid()) {
        _dropWidget->setShowDropIndicator(false);
    }
}

QString DVRViewPlugin::getVolumeDataSetID() const
{
    if (_volumeDataset.isValid())
        return _volumeDataset->getId();
    else
        return QString{};
}

QString DVRViewPlugin::getTfDatasetID() const
{
    if (_tfTexture.isValid())
        return _tfTexture->getId();
    else
        return QString{};
}

QString DVRViewPlugin::getReducedPosDataSetID() const
{
    if (_reducedPosDataset.isValid())
        return _reducedPosDataset->getId();
    else
        return QString{};
}

QString DVRViewPlugin::getMaterialTransitionDataSetID() const
{
    if (_materialTransitionTexture.isValid())
        return _materialTransitionTexture->getId();
    else
        return QString{};
}

QString DVRViewPlugin::getMaterialPositionsDataSetID() const
{
    if (_materialPositionTexture.isValid())
        return _materialPositionTexture->getId();
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
