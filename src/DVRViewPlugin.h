#pragma once

#include <ViewPlugin.h>
#include <Dataset.h>
#include <widgets/DropWidget.h>
#include <PointData/PointData.h>
#include <ColorData/ColorData.h>

#include <actions/PluginStatusBarAction.h>

#include "SettingsAction.h"

#include <QWidget>


using namespace mv::plugin;
using namespace mv::gui;
using namespace mv::util;

class DVRWidget;

/**
 * DVR view plugin class
 */
class DVRViewPlugin : public ViewPlugin
{
    Q_OBJECT

public:

    /**
     * Constructor
     * @param factory Pointer to the plugin factory
     */
    DVRViewPlugin(const PluginFactory* factory);

    /** Destructor */
    ~DVRViewPlugin() override = default;
    
    /** Get the ID of the dataset: usfull to compare agaist duplicates or which datasets to update*/
    QString getValueDataSetID() const;
    QString getSpatialDataSetID() const;

    /** This function is called by the core after the view plugin has been created */
    void init() override;

    void updateData();

    /** Retrieves data to be shown and updates the OpenGL plot */
    void renderData();

    bool checkDatasetIsValid();

    /** Store a private reference to the data set that should be displayed */
    void loadSpatialData(const mv::Datasets& datasets);
    void loadValueData(const mv::Datasets& datasets);

    /** Load data based on the global parameter */
    void loadData(const mv::Datasets& datasets) override;

private:
    /** We create and publish some data in order to provide an self-contained DVR project */
    void createData();
    std::vector<int> getNumbersUpTo(int number);
    void updateUI();

protected:
    DropWidget*                         _dropWidget;            /** Widget for drag and drop behavior */
    DVRWidget*                          _DVRWidget;             /** The OpenGL widget */
    SettingsAction                      _settingsAction;        /** Settings action */
    Dataset<Points>                     _spatialDataSet;        /** Points smart pointer to spatial location */
    Dataset<Points>                     _valueDataSet;          /** Points smart pointer to values */
    std::vector<unsigned int>           _currentDimensions;     /** Stores which dimensions of the current data are shown */
    bool                                _loadSpatial;           /** Flag to indicate which dataset to load */
};

/**
 * DVR view plugin factory class
 *
 * Note: Factory does not need to be altered (merely responsible for generating new plugins when requested)
 */
class DVRViewPluginFactory : public ViewPluginFactory
{
    Q_INTERFACES(mv::plugin::ViewPluginFactory mv::plugin::PluginFactory)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID   "studio.manivault.DVRViewPlugin"
                      FILE  "DVRViewPlugin.json")

public:

    /** Default constructor */
    DVRViewPluginFactory();

    /** Destructor */
    ~DVRViewPluginFactory() override {}

    /** Perform post-construction initialization */
    void initialize() override;

    /** Get plugin icon */
    QIcon getIcon(const QColor& color = Qt::black) const override;

    /** Creates an instance of the DVR view plugin */
    ViewPlugin* produce() override;

    /** Returns the data types that are supported by the DVR view plugin */
    mv::DataTypes supportedDataTypes() const override;

    /**
     * Get plugin trigger actions given \p datasets
     * @param datasets Vector of input datasets
     * @return Vector of plugin trigger actions
     */
    PluginTriggerActions getPluginTriggerActions(const mv::Datasets& datasets) const override;

private:
    PluginStatusBarAction*  _statusBarAction;               /** For global action in a status bar */
    HorizontalGroupAction   _statusBarPopupGroupAction;     /** Popup group action for status bar action */
    StringAction            _statusBarPopupAction;          /** Popup action for the status bar */
};
