#pragma once

#include <ViewPlugin.h>

#include <actions/HorizontalToolbarAction.h>
#include <graphics/Vector2f.h>
#include <actions/PluginStatusBarAction.h>

#include "SettingsAction.h"

#include <QTimer>


using namespace mv::plugin;
using namespace mv::gui;
using namespace mv::util;


class Points;
class TransferFunctionWidget;

namespace mv
{
    namespace gui {
        class DropWidget;
    }
}

/**
 * DRV TransferFunction plugin class
 */
class DVRTransferFunction : public ViewPlugin
{
    Q_OBJECT

public:

    /**
     * Constructor
     * @param factory Pointer to the plugin factory
     */
    DVRTransferFunction(const PluginFactory* factory);
    ~DVRTransferFunction() override;
    
    /** This function is called by the core after the view plugin has been created */
    void init() override;

    /** Store a private reference to the data set that should be displayed */
    void loadData(const Datasets datasets);

    /**  Updates the render settings */
    void updatePlot();
    void updateData();
    void updateSelection();

	bool isDataValid() const;


public: // Miscellaneous
    /** Get smart pointer to points dataset for point position */
    Dataset<Points>& getPositionDataset();

    /** Use the pixel selection tool to select data points */
    void selectPoints();

    /** Use the sampler pixel selection tool to sample data points */
    void samplePoints();

public:
    TransferFunctionWidget& getTransferFunctionWidget() { return *_TFWidget; };

    SettingsAction& getSettingsAction() { return _settingsAction; }
private:
    QString getCurrentDataSetID() const;

protected:
    DropWidget*                 _dropWidget;            /** Widget for drag and drop behavior */
    TransferFunctionWidget*     _TFWidget;              /** The OpenGL widget */
	//MaterialPickerAction*       _materialPickerAction;  /** Material picker UI */
    SettingsAction              _settingsAction;        /** Settings action */
    Dataset<Points>             _pointsDataset;         /** Points smart pointer */
    HorizontalToolbarAction     _primaryToolbarAction;  /** Horizontal toolbar for primary content */

	std::vector<mv::Vector2f>   _positions;             /** Vector of 2D points */
	std::int32_t                _numPoints;             /** Number of points */
};

/**
 * DRV TransferFunction plugin factory class
 *
 * Note: Factory does not need to be altered (merely responsible for generating new plugins when requested)
 */
class DVRTransferFunctionFactory : public ViewPluginFactory
{
    Q_INTERFACES(mv::plugin::ViewPluginFactory mv::plugin::PluginFactory)
        Q_OBJECT
        Q_PLUGIN_METADATA(IID   "studio.manivault.DVRTransferFunction"
            FILE  "DVRTransferFunction.json")

public:
    DVRTransferFunctionFactory();

    /**
     * Get plugin icon
     * @param color Icon color for flat (font) icons
     * @return Icon
     */
    QIcon getIcon(const QColor& color = Qt::black) const override;

    ViewPlugin* produce() override;

    /**
     * Get plugin trigger actions given \p datasets
     * @param datasets Vector of input datasets
     * @return Vector of plugin trigger actions
     */
    PluginTriggerActions getPluginTriggerActions(const mv::Datasets& datasets) const override;

    /**
     * Get the URL of the GitHub repository
     * @return URL of the GitHub repository (or readme markdown URL if set)
     */
    QUrl getRepositoryUrl() const override;
};