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
    return fileName;
}

void DVRVolumeLoader::loadData()
{
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

QIcon DVRVolumeLoaderFactory::getIcon(const QColor& color /*= Qt::black*/) const
{
    return Application::getIconFont("FontAwesome").getIcon("database");
}

// =============================================================================
// Factory
// =============================================================================

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

DVRVolumeLoadingInputDialog::DVRVolumeLoadingInputDialog(QWidget* parent, DVRVolumeLoader& dvrVolumeLoader) :
    QDialog(parent),
    _datasetNameAction(this, "Dataset name", QString("Enter Name")),
    _dataTypeAction(this, "Data type", { "Float", "Unsigned Byte" }),
    _numberOfDimensionsAction(this, "Number of dimensions", 1, 1000000, 1),
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

    _numberOfDimensionsAction.setDefaultWidgetFlags(IntegralAction::WidgetFlag::SpinBox);

    QStringList pointDataTypes;
    for (const char* const typeName : PointData::getElementTypeNames())
    {
        pointDataTypes.append(QString::fromLatin1(typeName));
    }
    _storeAsAction.setOptions(pointDataTypes);

    // Load some settings
    _dataTypeAction.setCurrentIndex(dvrVolumeLoader.getSetting("DataType").toInt());
    _numberOfDimensionsAction.setValue(dvrVolumeLoader.getSetting("NumberOfDimensions").toInt());
    _storeAsAction.setCurrentIndex(dvrVolumeLoader.getSetting("StoreAs").toInt());


    _settingsGroupAction.addAction(&_dataTypeAction);
    _settingsGroupAction.addAction(&_numberOfDimensionsAction);
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
        if (_datasetSource == DatasetSource::File) {
            QWidget* temp = _fileGroupAction.createWidget(this);
            layout->replaceWidget(_selectedWidget, temp);
            _selectedWidget = temp;
        }
        else if (_datasetSource == DatasetSource::PointDatasets)
        {
            QWidget* temp = _datasetGroupAction.createWidget(this);
            layout->replaceWidget(_selectedWidget, temp);
            _selectedWidget = temp;
        }
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
        dvrVolumeLoader.setSetting("NumberOfDimensions", _numberOfDimensionsAction.getValue());
        dvrVolumeLoader.setSetting("StoreAs", _storeAsAction.getCurrentIndex());

        accept();
    });
}
