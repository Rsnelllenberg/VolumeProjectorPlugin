#pragma once

#include <ViewPlugin.h>
#include <Dataset.h>
#include <widgets/DropWidget.h>
#include <PointData/PointData.h>

#include <actions/PluginStatusBarAction.h>

#include "SettingsAction.h"

#include <QWidget>

/** All plugin related classes are in the ManiVault plugin namespace */
using namespace mv::plugin;

/** Drop widget used in this plugin is located in the ManiVault gui namespace */
using namespace mv::gui;

/** Dataset reference used in this plugin is located in the ManiVault util namespace */
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
    
    /** This function is called by the core after the view plugin has been created */
    void init() override;

    /** Store a private reference to the data set that should be displayed */
    void loadData(const mv::Dataset<Points>& datasets);

    /**  Updates the render settings */
    void updatePlot();

    void updateData();

private:
    /** We create and publish some data in order to provide an self-contained DVR project */
    void createData();

    QString getCurrentDataSetID() const;

protected:
    DropWidget*                 _dropWidget;            /** Widget for drag and drop behavior */
    DVRWidget*                  _DVRWidget;       /** The OpenGL widget */
    SettingsAction              _settingsAction;        /** Settings action */
    mv::Dataset<Points>         _spatialDataset;        /** Points smart pointer */
    mv::Dataset<Points>         _valueDataset;        /** Points smart pointer */
    std::vector<unsigned int>   _currentDimensions;     /** Stores which dimensions of the current data are shown */
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
