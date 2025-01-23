#include "DVRVolumeLoader.h"

#include <PointData/PointData.h>

#include <Set.h>

#include <QtCore>
#include <QtDebug>

#include <cstdlib>
#include <fstream>
#include <type_traits>
#include <vector>

Q_PLUGIN_METADATA(IID "nl.tudelft.DVRVolumeLoader")

using namespace mv;
using namespace mv::gui;

// =============================================================================
// View
// =============================================================================

DVRVolumeLoader::~DVRVolumeLoader(void)
{

}

void DVRVolumeLoader::init()
{

}

namespace {


template <typename T, typename S>
void readDataAndAddToCore(mv::Dataset<Points>& point_data, int32_t numDims, const std::vector<char>& contents)
{

    // convert binary data to float vector
    std::vector<S> data;
    auto add_to_data = [&data](auto val) ->void {
        auto c = static_cast<S>(val);
        data.push_back(c);
    };

    if constexpr (std::is_same_v<T, float>) {
        for (size_t i = 0; i < contents.size() / 4; i++)
        {
            float f = ((float*)contents.data())[i];
            add_to_data(f);
        }
    }
    else if constexpr (std::is_same_v<T, unsigned char>)
    {
        for (size_t i = 0; i < contents.size(); i++)
        {
            T c = static_cast<T>(contents[i]);
            add_to_data(c);
        }
    }
    else
    {
        qWarning() << "DVRVolumeLoader.cpp::readDataAndAddToCore: No data loaded. Template typename not implemented.";
    }

    if(std::lldiv(static_cast<long long>(data.size()), static_cast<long long>(numDims)).rem != 0)
        qWarning() << "WARNING: DVRVolumeLoader.cpp::readDataAndAddToCore: Data size divided by number of dimension is not an integer. Something might have gone wrong.";

    // add data to the core
    point_data->setData(std::move(data), numDims);
    events().notifyDatasetDataChanged(point_data);

    qDebug() << "Number of dimensions: " << point_data->getNumDimensions();
    qDebug() << "BIN file loaded. Num data points: " << point_data->getNumPoints();

}

// Recursively searches for the data element type that is specified by the selectedDataElementType parameter. 
template <typename T, unsigned N = 0>
void recursiveReadDataAndAddToCore(const QString& selectedDataElementType, mv::Dataset<Points>& point_data, int32_t numDims, const std::vector<char>& contents)
{
    const QLatin1String nthDataElementTypeName(std::get<N>(PointData::getElementTypeNames()));

    if (selectedDataElementType == nthDataElementTypeName)
    {
        readDataAndAddToCore<T, PointData::ElementTypeAt<N>>(point_data, numDims, contents);
    }
    else
    {
        recursiveReadDataAndAddToCore<T, N + 1>(selectedDataElementType, point_data, numDims, contents);
    }
}

template <>
void recursiveReadDataAndAddToCore<float, PointData::getNumberOfSupportedElementTypes()>(const QString&, mv::Dataset<Points>&, int32_t, const std::vector<char>&)
{
    // This specialization does nothing, intensionally! 
}

template <>
void recursiveReadDataAndAddToCore<unsigned char, PointData::getNumberOfSupportedElementTypes()>(const QString&, mv::Dataset<Points>&, int32_t, const std::vector<char>&)
{
    // This specialization does nothing, intensionally! 
}

}

QString DVRVolumeLoader::getFile()
{
    QString fileName = AskForFileName(tr("BIN Files (*.bin)"));

    // Don't try to load a file if the dialog was cancelled or the file name is empty
    if (fileName.isNull() || fileName.isEmpty())
        return QString();

    qDebug() << "Loading BIN file: " << fileName;

    // read in binary data

    std::ifstream in(fileName.toStdString(), std::ios::in | std::ios::binary);
    if (in)
    {
        in.seekg(0, std::ios::end);
        _contents.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&_contents[0], _contents.size());
        in.close();
    }
    else
    {
        throw DataLoadException(fileName, "File was not found at location.");
    }
    return QFileInfo(fileName).baseName();
}

void DVRVolumeLoader::loadData()
{
    createData(); // Create some example data for debugging purposes
    DVRVolumeLoadingInputDialog* inputDialog = new DVRVolumeLoadingInputDialog(nullptr, *this);

    connect(inputDialog, &QDialog::accepted, this, [this, inputDialog]() -> void {
        if (!inputDialog->getDatasetName().isEmpty()) {

            auto sourceDataset = inputDialog->getSourceDataset();
            auto numDims = inputDialog->getNumberOfDimensions();
            auto storeAs = inputDialog->getStoreAs();

            Dataset<Points> point_data;

            if (sourceDataset.isValid())
                point_data = mv::data().createDerivedDataset<Points>(inputDialog->getDatasetName(), sourceDataset);
            else
                point_data = mv::data().createDataset<Points>("Points", inputDialog->getDatasetName());

            if (inputDialog->getDataType() == BinaryDataType::FLOAT)
            {
                recursiveReadDataAndAddToCore<float>(storeAs, point_data, numDims, _contents);
            }
            else if (inputDialog->getDataType() == BinaryDataType::UBYTE)
            {
                recursiveReadDataAndAddToCore<unsigned char>(storeAs, point_data, numDims, _contents);
            }


        }
        });
    inputDialog->open();
}

void DVRVolumeLoader::createData()
{
    // Here, we create a random data set, so that we do not need 
    // to use other plugins for loading when trying out this example
    auto points = mv::data().createDataset<Points>("Points", "DVRViewData");

    int numPoints = 1000;
    const std::vector<QString> dimNames{ "Dim v1", "Dim v2", "Dim v3", "Dim 4", "Dim5", "Dim6", "Dim7", "Dim8" };
    //const std::vector<QString> dimNames{ "Dim x", "Dim y", "Dim z", "Dim v1", "Dim v2", "Dim v3", "Dim 4"};
    int numDimensions = dimNames.size() + 3;

    qDebug() << "DVRViewPlugin::createData: Create some example data. " << numPoints << " points, each with " << numDimensions << " dimensions";

    // Create random example data
    std::vector<float> exampleData;
    std::vector<float> spatialData;
    {
        std::default_random_engine generator;
        std::uniform_real_distribution<float> distribution(0.0, 1.0);
        int totalPoints = numPoints * numDimensions;
        for (int i = 0; i < totalPoints; i++)
        {
            float value = distribution(generator);
            int index = i % numDimensions;
            if (index == 0) {
                value = 0.5;
                if (i == 0) {
                    value = 0.0;
                }
            }
            if (index <= 2) {
                spatialData.push_back(value);
            }
            else {
                exampleData.push_back(value);
            }
            //exampleData.push_back(i / totalPoints);
            //qDebug() << "exampleData[" << i << "]: " << exampleData[i];
        }
    }

    // Passing example data 
    points->setData(exampleData.data(), numPoints, dimNames.size());
    points->setDimensionNames(dimNames);

    qDebug() << "spatialData size:" << spatialData.size();
    // Create a dirived spatial dataset
    const std::vector<QString> dimNames2 = { "Dim x", "Dim y", "Dim z" };
    //auto spatial = mv::data().createDataset<Points>("Points", "Spatial", points);
    auto spatial = mv::data().createDerivedDataset<Points>("Spatial", points);
    spatial->setData(spatialData.data(), numPoints, 3);
    spatial->setDimensionNames(dimNames2);

    std::vector<float> spatialdata(numPoints * 3);
    spatial->populateDataForDimensions(spatialdata, spatial->indices);

    for (int i = 0; i < spatialdata.size(); i++) {
        qDebug() << "DVRViewPlugin::updateData: spatialData[" << i << "]: " << spatialData[i];
    }

    //std::vector<float> valuedata(numPoints * dimNames.size());
    //points->populateDataForDimensions(valuedata, generateSequence(dimNames.size()));

    //for (int i = 0; i < valuedata.size(); i++) {
    //    qDebug() << "DVRViewPlugin::updateData: valuedata[" << i << "]: " << valuedata[i];
    //}

    // Notify the core system of the new data
    events().notifyDatasetDataChanged(points);
    events().notifyDatasetDataDimensionsChanged(points);
    events().notifyDatasetDataChanged(spatial);
    events().notifyDatasetDataDimensionsChanged(spatial);
}

// =============================================================================
// Factory
// =============================================================================

QIcon DVRVolumeLoaderFactory::getIcon(const QColor& color /*= Qt::black*/) const
{
    return Application::getIconFont("FontAwesome").getIcon("cube");
}

LoaderPlugin* DVRVolumeLoaderFactory::produce()
{
    return new DVRVolumeLoader(this);
}

DataTypes DVRVolumeLoaderFactory::supportedDataTypes() const
{
    DataTypes supportedTypes;
    supportedTypes.append(PointType);
    return supportedTypes;
}

// =============================================================================
// UI
// =============================================================================

DVRVolumeLoadingInputDialog::DVRVolumeLoadingInputDialog(QWidget* parent, DVRVolumeLoader& dvrVolumeLoader) :
    QDialog(parent),
    _datasetNameAction(this, "Dataset name", QString("Enter Name")),
    _dataTypeAction(this, "Data type", { "Float", "Unsigned Byte" }),
    _numberOfValueDimensionsAction(this, "Number of dimensions (Values)", 1, 1000000, 1),
    _numberOfDimensionsXAction(this, "Number of dimensions (X)", 1, 1000000, 1),
    _numberOfDimensionsYAction(this, "Number of dimensions (Y)", 1, 1000000, 1),
    _numberOfDimensionsZAction(this, "Number of dimensions (Z)", 1, 1000000, 1),
    _storeAsAction(this, "Store as"),
    _isDerivedAction(this, "Mark as derived", false),
    _sourceDatasetPickerAction(this, "Source dataset"),
    _spatialDatasetPickerAction(this, "Spatial dataset"),
    _valueDatasetPickerAction(this, "Value dataset"),
    _acceptAction(this, "Accept"),
    _fileLoadAction(this, "Load File"),
    _settingsGroupAction(this, "Settings"),
    _fileGroupAction(this, "File selection"),
    _datasetGroupAction(this, "Dataset selection"),
    _selectedWidget(nullptr)
{
    setWindowTitle(tr("DVRVolume Loader"));

    _numberOfDimensionsXAction.setDefaultWidgetFlags(IntegralAction::WidgetFlag::SpinBox);
    _numberOfDimensionsYAction.setDefaultWidgetFlags(IntegralAction::WidgetFlag::SpinBox);
    _numberOfDimensionsZAction.setDefaultWidgetFlags(IntegralAction::WidgetFlag::SpinBox);
    _numberOfValueDimensionsAction.setDefaultWidgetFlags(IntegralAction::WidgetFlag::SpinBox);

    QStringList pointDataTypes;
    for (const char* const typeName : PointData::getElementTypeNames())
    {
        pointDataTypes.append(QString::fromLatin1(typeName));
    }
    _storeAsAction.setOptions(pointDataTypes);

    // Load some settings
    _dataTypeAction.setCurrentIndex(dvrVolumeLoader.getSetting("DataType").toInt());
    _numberOfValueDimensionsAction.setValue(dvrVolumeLoader.getSetting("NumberOfValueDimensions").toInt());
    _numberOfDimensionsXAction.setValue(dvrVolumeLoader.getSetting("NumberOfDimensionsX").toInt());
    _numberOfDimensionsYAction.setValue(dvrVolumeLoader.getSetting("NumberOfDimensionsY").toInt());
    _numberOfDimensionsZAction.setValue(dvrVolumeLoader.getSetting("NumberOfDimensionsZ").toInt());
    _storeAsAction.setCurrentIndex(dvrVolumeLoader.getSetting("StoreAs").toInt());

    _settingsGroupAction.addAction(&_dataTypeAction);
    _settingsGroupAction.addAction(&_numberOfValueDimensionsAction);
    _settingsGroupAction.addAction(&_numberOfDimensionsXAction);
    _settingsGroupAction.addAction(&_numberOfDimensionsYAction);
    _settingsGroupAction.addAction(&_numberOfDimensionsZAction);
    _settingsGroupAction.addAction(&_storeAsAction);
    _settingsGroupAction.addAction(&_isDerivedAction);
    _settingsGroupAction.addAction(&_sourceDatasetPickerAction);

    // Add radio buttons for dataset source selection
    _fileRadioButton = new QRadioButton(tr("File"), this);
    _pointDatasetsRadioButton = new QRadioButton(tr("Point Datasets"), this);

    _dataSourceButtonGroup = new QButtonGroup(this);
    _dataSourceButtonGroup->addButton(_fileRadioButton, DatasetSource::File);
    _dataSourceButtonGroup->addButton(_pointDatasetsRadioButton, DatasetSource::PointDatasets);

    _fileRadioButton->setChecked(true); // Default to None

    auto layout = new QVBoxLayout();

    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(_settingsGroupAction.createWidget(this));

    auto buttonsLayout = new QHBoxLayout();
    buttonsLayout->setContentsMargins(150, 0, 0, 0);
    buttonsLayout->addWidget(_fileRadioButton);
    buttonsLayout->addWidget(_pointDatasetsRadioButton);
    layout->addLayout(buttonsLayout);

    _fileGroupAction.addAction(&_fileLoadAction);
    _fileGroupAction.addAction(&_datasetNameAction);
    _fileGroupAction.addAction(&_acceptAction);

    auto dataSets = mv::data().getAllDatasets(std::vector<mv::DataType> {PointType});
    _spatialDatasetPickerAction.setDatasets(dataSets);
    _valueDatasetPickerAction.setDatasets(dataSets);

    _datasetGroupAction.addAction(&_spatialDatasetPickerAction);
    _datasetGroupAction.addAction(&_valueDatasetPickerAction);
    _datasetGroupAction.addAction(&_acceptAction);

    _selectedWidget = _fileGroupAction.createWidget(this);
    layout->addWidget(_selectedWidget);
    setLayout(layout);

    // Add functionality to the file button
    connect(&_fileLoadAction, &TriggerAction::triggered, &dvrVolumeLoader, [this, &dvrVolumeLoader]() -> void {
        _datasetNameAction.setString(dvrVolumeLoader.getFile());
        });

    //Update the selected widget when a radio button is clicked
    connect(_dataSourceButtonGroup, &QButtonGroup::buttonClicked, this, [this, layout]() -> void {
        int id = _dataSourceButtonGroup->checkedId();
        _datasetSource = static_cast<DatasetSource>(id);
        if (_selectedWidget) {
            layout->removeWidget(_selectedWidget);
            _selectedWidget->deleteLater();
        }
        if (_datasetSource == DatasetSource::File) {
            _selectedWidget = _fileGroupAction.createWidget(this);
        }
        else if (_datasetSource == DatasetSource::PointDatasets) {
            _selectedWidget = _datasetGroupAction.createWidget(this);
        }
        layout->addWidget(_selectedWidget);
        layout->update();
        });

    // Update the state of the dataset picker
    const auto updateDatasetPicker = [this]() -> void {
        if (_isDerivedAction.isChecked()) {

            // Get unique identifier and gui names from all point data sets in the core
            auto dataSets = mv::data().getAllDatasets(std::vector<mv::DataType> {PointType});
            // Assign found dataset(s)
            _sourceDatasetPickerAction.setDatasets(dataSets);
        }
        else {

            // Assign found dataset(s)
            _sourceDatasetPickerAction.setDatasets(mv::Datasets());
        }

        // Disable dataset picker when not marked as derived
        _sourceDatasetPickerAction.setEnabled(_isDerivedAction.isChecked());
        };

    // Populate source datasets once the dataset is marked as derived
    connect(&_isDerivedAction, &ToggleAction::toggled, this, updateDatasetPicker);

    // Update dataset picker at startup
    updateDatasetPicker();

    // Accept when the load action is triggered
    connect(&_acceptAction, &TriggerAction::triggered, this, [this, &dvrVolumeLoader]() {

        // Save some settings
        dvrVolumeLoader.setSetting("DataType", _dataTypeAction.getCurrentIndex());
        dvrVolumeLoader.setSetting("NumberOfValueDimensions", _numberOfValueDimensionsAction.getValue());
        dvrVolumeLoader.setSetting("NumberOfDimensionsX", _numberOfDimensionsXAction.getValue());
        dvrVolumeLoader.setSetting("NumberOfDimensionsY", _numberOfDimensionsYAction.getValue());
        dvrVolumeLoader.setSetting("NumberOfDimensionsZ", _numberOfDimensionsZAction.getValue());
        dvrVolumeLoader.setSetting("StoreAs", _storeAsAction.getCurrentIndex());

        accept();
    });
}

