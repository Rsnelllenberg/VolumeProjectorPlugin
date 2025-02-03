#pragma once

#include <actions/DatasetPickerAction.h>
#include <actions/GroupAction.h>
#include <actions/IntegralAction.h>
#include <actions/OptionAction.h>
#include <actions/StringAction.h>
#include <actions/TriggerAction.h>

#include <LoaderPlugin.h>

#include <QDialog>
#include <QRadioButton>
#include <QButtonGroup>

#include <VolumeData/Volumes.h>

using namespace mv::plugin;

// =============================================================================
// Loading input box
// =============================================================================

class DVRVolumeLoader;

enum BinaryDataType
{
    FLOAT, UBYTE, UINT16
};

enum DatasetSource
{
    File, PointDatasets
};

class DVRVolumeLoadingInputDialog : public QDialog
{
    Q_OBJECT

public:
    DVRVolumeLoadingInputDialog(QWidget* parent, DVRVolumeLoader& dvrVolumeLoader);

    /** Get preferred size */
    QSize sizeHint() const override {
        return QSize(400, 50);
    }

    /** Get minimum size hint*/
    QSize minimumSizeHint() const override {
        return sizeHint();
    }

    /** Get the GUI name of the loaded dataset */
    QString getDatasetName() const {
        return _datasetNameAction.getString();
    }

    DatasetSource getDatasetSource() const {
        return _datasetSource;
    }

    /** Get the binary data type */
    BinaryDataType getDataType() const {
        if (_dataTypeAction.getCurrentIndex() == 0) // Float32
            return BinaryDataType::FLOAT;
        else if (_dataTypeAction.getCurrentIndex() == 1) // Unsigned Int16
            return BinaryDataType::UINT16;
        else // Unsigned Byte
            return BinaryDataType::UBYTE;
    }

    /** Get the number of dimensions */
    std::int32_t getNumberOfValueDimensions() const {
        return _numberOfValueDimensionsAction.getValue();
    }

    /** Get the number of dimensions on the x-axis */
    std::int32_t getNumberOfDimensionsX() const {
        return _numberOfDimensionsXAction.getValue();
    }

    /** Get the number of dimensions on the y-axis */
    std::int32_t getNumberOfDimensionsY() const {
        return _numberOfDimensionsYAction.getValue();
    }

    /** Get the number of dimensions on the z-axis */
    std::int32_t getNumberOfDimensionsZ() const {
        return _numberOfDimensionsZAction.getValue();
    }

    /** Get the desired storage type */
    QString getStoreAs() const {
        return _storeAsAction.getCurrentText();
    }

    /** Get whether the dataset will be marked as derived */
    bool getIsDerived() const {
        return _isDerivedAction.isChecked();
    }

    /** Get smart pointer to dataset (if any) */
    mv::Dataset<mv::DatasetImpl> getSourceDataset() const {
        return _sourceDatasetPickerAction.getCurrentDataset();
    }

    mv::Dataset<mv::DatasetImpl> getSpatialDataset() const {
        return _spatialDatasetPickerAction.getCurrentDataset();
    }

    mv::Dataset<mv::DatasetImpl> getValueDataset() const {
        return _valueDatasetPickerAction.getCurrentDataset();
    }

protected:
    mv::gui::StringAction            _datasetNameAction;             /** Dataset name action */
    mv::gui::OptionAction            _dataTypeAction;                /** Data type action */
    mv::gui::IntegralAction          _numberOfValueDimensionsAction; /** Number of dimensions action */
    mv::gui::IntegralAction          _numberOfDimensionsXAction;     /** Number of dimensions on x-axis action */
    mv::gui::IntegralAction          _numberOfDimensionsYAction;     /** Number of dimensions on y-axis action */
    mv::gui::IntegralAction          _numberOfDimensionsZAction;     /** Number of dimensions on z-axis action */
    mv::gui::OptionAction            _storeAsAction;                 /** Store as action */
    mv::gui::ToggleAction            _isDerivedAction;               /** Mark dataset as derived action */
    mv::gui::DatasetPickerAction     _sourceDatasetPickerAction;     /** Dataset picker action for picking source datasets */
    mv::gui::DatasetPickerAction     _spatialDatasetPickerAction;    /** Dataset picker action for picking spatial datasets */
    mv::gui::DatasetPickerAction     _valueDatasetPickerAction;      /** Dataset picker action for picking value datasets */
    mv::gui::TriggerAction           _acceptAction;                  /** Load action */
    mv::gui::TriggerAction           _fileLoadAction;                /** File action */
    mv::gui::GroupAction             _settingsGroupAction;           /** Shared group action */
    mv::gui::GroupAction             _fileGroupAction;               /** File specific group action */
    mv::gui::GroupAction             _datasetGroupAction;            /** Datasets specific group action */

    DatasetSource                    _datasetSource;                 /** Dataset source */

    QRadioButton*                   _fileRadioButton;                /** File radio button */
    QRadioButton*                   _pointDatasetsRadioButton;       /** Point datasets radio button */
    QButtonGroup*                   _dataSourceButtonGroup;          /** Data source button group */

    QWidget*                        _selectedWidget;                 /** File widget */
};


// =============================================================================
// View
// =============================================================================

class DVRVolumeLoader : public LoaderPlugin
{
    Q_OBJECT
public:
    DVRVolumeLoader(const PluginFactory* factory) : LoaderPlugin(factory) { }
    ~DVRVolumeLoader(void) override;

    void init() override;

    void loadData() Q_DECL_OVERRIDE;
    QString getFile();

protected:
    std::vector<char> _contents;
    mv::Vector3f normalizePosition(const mv::Vector3f& pos, mv::Vector3f min, mv::Vector3f max, Size3D size);
    mv::Dataset<Volumes>            _volumesDataset;                 /** Volumes dataset */

};


// =============================================================================
// Factory
// =============================================================================

class DVRVolumeLoaderFactory : public LoaderPluginFactory
{
    Q_INTERFACES(mv::plugin::LoaderPluginFactory mv::plugin::PluginFactory)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID   "nl.tudelft.DVRVolumeLoader"
                      FILE  "DVRVolumeLoader.json")

public:
    DVRVolumeLoaderFactory(void) {}
    ~DVRVolumeLoaderFactory(void) override {}

    /**
     * Get plugin icon
     * @param color Icon color for flat (font) icons
     * @return Icon
     */
    QIcon getIcon(const QColor& color = Qt::black) const override;

    LoaderPlugin* produce() override;

    mv::DataTypes supportedDataTypes() const override;
};