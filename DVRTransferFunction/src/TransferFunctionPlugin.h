#pragma once

#include <ViewPlugin.h>

#include <actions/HorizontalToolbarAction.h>
#include <graphics/Vector2f.h>

#include "SettingsAction.h"

#include <QTimer>

using namespace mv::plugin;
using namespace mv::util;
using namespace mv::gui;

class Points;
class TransferFunctionWidget;

namespace mv
{
    namespace gui {
        class DropWidget;
    }
}

class TransferFunctionPlugin : public ViewPlugin
{
    Q_OBJECT

public:
    TransferFunctionPlugin(const PluginFactory* factory);
    ~TransferFunctionPlugin() override;

    void init() override;

    /**
     * Load one (or more datasets in the view)
     * @param datasets Dataset(s) to load
     */
    void loadData(const Datasets& datasets) override;

    /** Get number of points in the position dataset */
    std::uint32_t getNumberOfPoints() const;

public:
    void createSubset(const bool& fromSourceData = false, const QString& name = "");

protected: // Data loading

    /** Invoked when the position points dataset changes */
    void positionDatasetChanged();

public: // Point colors

    /**
     * Load color from points dataset
     * @param points Smart pointer to points dataset
     * @param dimensionIndex Index of the dimension to load
     */
    void loadColors(const Dataset<Points>& points, const std::uint32_t& dimensionIndex);

public: // Miscellaneous

    /** Get smart pointer to points dataset for point position */
    Dataset<Points>& getPositionDataset();

    /** Use the pixel selection tool to select data points */
    void selectPoints();

    /** Use the sampler pixel selection tool to sample data points */
    void samplePoints();

public:

    /** Get reference to the scatter plot widget */
    TransferFunctionWidget& getTransferFunctionWidget();

    SettingsAction& getSettingsAction() { return _settingsAction; }

private:
    void updateData();
    void updateSelection();

public: // Serialization

    /**
     * Load plugin from variant map
     * @param variantMap Variant map representation of the plugin
     */
    void fromVariantMap(const QVariantMap& variantMap) override;

    /**
     * Save plugin to variant map
     * @return Variant map representation of the plugin
     */
    QVariantMap toVariantMap() const override;

private:
    mv::gui::DropWidget*            _dropWidget;                /** Widget for dropping datasets */
    TransferFunctionWidget*         _transferFunctionWidget;         /** THe visualization widget */

    Dataset<Points>                 _positionDataset;           /** Smart pointer to points dataset for point position */
    std::vector<mv::Vector2f>       _positions;                 /** Point positions */
    unsigned int                    _numPoints;                 /** Number of point positions */

    SettingsAction                  _settingsAction;            /** Group action for all settings */
    HorizontalToolbarAction         _primaryToolbarAction;      /** Horizontal toolbar for primary content */

    static const std::int32_t LAZY_UPDATE_INTERVAL = 2;

};

// =============================================================================
// Factory
// =============================================================================

class TransferFunctionPluginFactory : public ViewPluginFactory
{
    Q_INTERFACES(mv::plugin::ViewPluginFactory mv::plugin::PluginFactory)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID   "studio.manivault.TransferFunctionPlugin"
                      FILE  "TransferFunctionPlugin.json")

public:
    TransferFunctionPluginFactory();

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