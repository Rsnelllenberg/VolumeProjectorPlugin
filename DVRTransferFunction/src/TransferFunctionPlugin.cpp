#include "TransferFunctionPlugin.h"

#include "TransferFunctionWidget.h"

#include <Application.h>
#include <DataHierarchyItem.h>

#include <util/PixelSelectionTool.h>
#include <util/Timer.h>

#include <ClusterData/ClusterData.h>
#include <ColorData/ColorData.h>
#include <PointData/PointData.h>

#include <graphics/Vector3f.h>

#include <widgets/DropWidget.h>
#include <widgets/ViewPluginLearningCenterOverlayWidget.h>

#include <actions/PluginTriggerAction.h>

#include <DatasetsMimeData.h>

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QMenu>
#include <QMetaType>
#include <QtCore>

#include <algorithm>
#include <functional>
#include <vector>
#include <actions/ViewPluginSamplerAction.h>

Q_PLUGIN_METADATA(IID "studio.manivault.TransferFunctionPlugin")

using namespace mv;
using namespace mv::util;

TransferFunctionPlugin::TransferFunctionPlugin(const PluginFactory* factory) :
    ViewPlugin(factory),
    _dropWidget(nullptr),
    _scatterPlotWidget(new TransferFunctionWidget()),
    _numPoints(0),
    _settingsAction(this, "Settings"),
    _primaryToolbarAction(this, "Primary Toolbar"),
    _secondaryToolbarAction(this, "Secondary Toolbar")
{
    setObjectName("TransferFunction");

    auto& shortcuts = getShortcuts();

    shortcuts.add({ QKeySequence(Qt::Key_R), "Selection", "Rectangle (default)" });
    shortcuts.add({ QKeySequence(Qt::Key_L), "Selection", "Lasso" });
    shortcuts.add({ QKeySequence(Qt::Key_B), "Selection", "Circular brush (mouse wheel adjusts the radius)" });
    shortcuts.add({ QKeySequence(Qt::SHIFT), "Selection", "Add to selection" });
    shortcuts.add({ QKeySequence(Qt::CTRL), "Selection", "Remove from selection" });

    shortcuts.add({ QKeySequence(Qt::Key_S), "Render", "Scatter mode (default)" });
    shortcuts.add({ QKeySequence(Qt::Key_D), "Render", "Density mode" });
    shortcuts.add({ QKeySequence(Qt::Key_C), "Render", "Contour mode" });

    shortcuts.add({ QKeySequence(Qt::ALT), "Navigation", "Pan (LMB down)" });
    shortcuts.add({ QKeySequence(Qt::ALT), "Navigation", "Zoom (mouse wheel)" });
    shortcuts.add({ QKeySequence(Qt::Key_O), "Navigation", "Original view" });
    
    _dropWidget = new DropWidget(_scatterPlotWidget);

    _scatterPlotWidget->getNavigationAction().setParent(this);

    getWidget().setFocusPolicy(Qt::ClickFocus);

    _primaryToolbarAction.addAction(&_settingsAction.getDatasetsAction());

    _primaryToolbarAction.addAction(&_settingsAction.getSelectionAction());
    _primaryToolbarAction.addAction(&getSamplerAction());

    connect(_scatterPlotWidget, &TransferFunctionWidget::customContextMenuRequested, this, [this](const QPoint& point) {
        if (!_positionDataset.isValid())
            return;

        auto contextMenu = _settingsAction.getContextMenu();

        contextMenu->addSeparator();

        _positionDataset->populateContextMenu(contextMenu);

        contextMenu->exec(getWidget().mapToGlobal(point));
    });

    _dropWidget->setDropIndicatorWidget(new DropWidget::DropIndicatorWidget(&getWidget(), "No data loaded", "Drag an item from the data hierarchy and drop it here to visualize data..."));
    _dropWidget->initialize([this](const QMimeData* mimeData) -> DropWidget::DropRegions {
        DropWidget::DropRegions dropRegions;

        const auto datasetsMimeData = dynamic_cast<const DatasetsMimeData*>(mimeData);

        if (datasetsMimeData == nullptr)
            return dropRegions;

        if (datasetsMimeData->getDatasets().count() > 1)
            return dropRegions;

        const auto& dataset         = datasetsMimeData->getDatasets().first();
        const auto datasetGuiName   = dataset->text();
        const auto datasetId        = dataset->getId();
        const auto dataType         = dataset->getDataType();
        const auto dataTypes        = DataTypes({ PointType , ColorType, ClusterType });

        // Check if the data type can be dropped
        if (!dataTypes.contains(dataType))
            dropRegions << new DropWidget::DropRegion(this, "Incompatible data", "This type of data is not supported", "exclamation-circle", false);

        // Points dataset is about to be dropped
        if (dataType == PointType) {

            // Get points dataset from the core
            auto candidateDataset = mv::data().getDataset<Points>(datasetId);

            // Establish drop region description
            const auto description = QString("Visualize %1 as points or density/contour map").arg(datasetGuiName);

            if (!_positionDataset.isValid()) {

                // Load as point positions when no dataset is currently loaded
                dropRegions << new DropWidget::DropRegion(this, "Point position", description, "map-marker-alt", true, [this, candidateDataset]() {
                    _positionDataset = candidateDataset;
                    });
            }
            else {
                if (_positionDataset != candidateDataset && candidateDataset->getNumDimensions() >= 2) {

                    // The number of points is equal, so offer the option to replace the existing points dataset
                    dropRegions << new DropWidget::DropRegion(this, "Point position", description, "map-marker-alt", true, [this, candidateDataset]() {
                        _positionDataset = candidateDataset;
                        });
                }

                if (candidateDataset->getNumPoints() == _positionDataset->getNumPoints()) {

                }
            }
        }

        // Cluster dataset is about to be dropped
        if (dataType == ClusterType) {

            // Get clusters dataset from the core
            auto candidateDataset = mv::data().getDataset<Clusters>(datasetId);

            // Establish drop region description
            const auto description = QString("Color points by %1").arg(candidateDataset->text());

            // Only allow user to color by clusters when there is a positions dataset loaded
            if (_positionDataset.isValid()) {

            }
            else {

                // Only allow user to color by clusters when there is a positions dataset loaded
                dropRegions << new DropWidget::DropRegion(this, "No points data loaded", "Clusters can only be visualized in concert with points data", "exclamation-circle", false);
            }
        }

        return dropRegions;
    });

    auto& selectionAction = _settingsAction.getSelectionAction();

    getSamplerAction().initialize(this, &selectionAction.getPixelSelectionAction(), &selectionAction.getSamplerPixelSelectionAction());
    getSamplerAction().setViewGeneratorFunction([this](const ViewPluginSamplerAction::SampleContext& toolTipContext) -> QString {
        QStringList localPointIndices, globalPointIndices;

        for (const auto& localPointIndex : toolTipContext["LocalPointIndices"].toList())
            localPointIndices << QString::number(localPointIndex.toInt());

        for (const auto& globalPointIndex : toolTipContext["GlobalPointIndices"].toList())
            globalPointIndices << QString::number(globalPointIndex.toInt());

        if (localPointIndices.isEmpty())
            return {};

        return  QString("<table> \
                    <tr> \
                        <td><b>Point ID's: </b></td> \
                        <td>%1</td> \
                    </tr> \
                   </table>").arg(globalPointIndices.join(", "));
    });

    //getSamplerAction().setViewingMode(ViewPluginSamplerAction::ViewingMode::Tooltip);
    getSamplerAction().getEnabledAction().setChecked(false);

    getLearningCenterAction().addVideos(QStringList({ "Practitioner", "Developer" }));
}

TransferFunctionPlugin::~TransferFunctionPlugin()
{
}

void TransferFunctionPlugin::init()
{
    auto layout = new QVBoxLayout();

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(_primaryToolbarAction.createWidget(&getWidget()));
    layout->addWidget(_scatterPlotWidget, 100);
    layout->addWidget(_secondaryToolbarAction.createWidget(&getWidget()));

    getWidget().setLayout(layout);

    // Update the data when the scatter plot widget is initialized
    connect(_scatterPlotWidget, &TransferFunctionWidget::initialized, this, &TransferFunctionPlugin::updateData);

    connect(&_scatterPlotWidget->getPixelSelectionTool(), &PixelSelectionTool::areaChanged, [this]() {
        if (_scatterPlotWidget->getPixelSelectionTool().isNotifyDuringSelection()) {
            selectPoints();
        }
    });

    connect(&_scatterPlotWidget->getPixelSelectionTool(), &PixelSelectionTool::ended, [this]() {
        if (_scatterPlotWidget->getPixelSelectionTool().isNotifyDuringSelection())
            return;

        selectPoints();
    });

    connect(&getSamplerAction(), &ViewPluginSamplerAction::sampleContextRequested, this, &TransferFunctionPlugin::samplePoints);

    connect(&_positionDataset, &Dataset<Points>::changed, this, &TransferFunctionPlugin::positionDatasetChanged);
    connect(&_positionDataset, &Dataset<Points>::dataChanged, this, &TransferFunctionPlugin::updateData);
    connect(&_positionDataset, &Dataset<Points>::dataSelectionChanged, this, &TransferFunctionPlugin::updateSelection);

    _scatterPlotWidget->installEventFilter(this);

    getLearningCenterAction().getViewPluginOverlayWidget()->setTargetWidget(_scatterPlotWidget);
}

void TransferFunctionPlugin::loadData(const Datasets& datasets)
{
    // Exit if there is nothing to load
    if (datasets.isEmpty())
        return;

    // Load the first dataset
    _positionDataset = datasets.first();
}

void TransferFunctionPlugin::createSubset(const bool& fromSourceData /*= false*/, const QString& name /*= ""*/)
{
    // Create the subset
    mv::Dataset<DatasetImpl> subset;

    if (fromSourceData)
        // Make subset from the source data, this is not the displayed data, so no restrictions here
        subset = _positionSourceDataset->createSubsetFromSelection(name, _positionSourceDataset);
    else
        // Avoid making a bigger subset than the current data by restricting the selection to the current data
        subset = _positionDataset->createSubsetFromVisibleSelection(name, _positionDataset);

    subset->getDataHierarchyItem().select();
}

void TransferFunctionPlugin::selectPoints()
{
    auto& pixelSelectionTool = _scatterPlotWidget->getPixelSelectionTool();

    // Only proceed with a valid points position dataset and when the pixel selection tool is active
    if (!_positionDataset.isValid() || !pixelSelectionTool.isActive() || _scatterPlotWidget->isNavigating())
        return;

    auto selectionAreaImage = pixelSelectionTool.getAreaPixmap().toImage();
    auto selectionSet       = _positionDataset->getSelection<Points>();

    std::vector<std::uint32_t> targetSelectionIndices;

    targetSelectionIndices.reserve(_positionDataset->getNumPoints());

    std::vector<std::uint32_t> localGlobalIndices;

    _positionDataset->getGlobalIndices(localGlobalIndices);

    auto& zoomRectangleAction = _scatterPlotWidget->getNavigationAction().getZoomRectangleAction();

    const auto width        = selectionAreaImage.width();
    const auto height       = selectionAreaImage.height();
    const auto size         = width < height ? width : height;
    const auto uvOffset     = QPoint((selectionAreaImage.width() - size) / 2.0f, (selectionAreaImage.height() - size) / 2.0f);

    QPointF uvNormalized    = {};
    QPoint uv               = {};

    for (std::uint32_t localPointIndex = 0; localPointIndex < _positions.size(); localPointIndex++) {
        uvNormalized = QPointF((_positions[localPointIndex].x - zoomRectangleAction.getLeft()) / zoomRectangleAction.getWidth(), (zoomRectangleAction.getTop() - _positions[localPointIndex].y) / zoomRectangleAction.getHeight());
        uv           = uvOffset + QPoint(uvNormalized.x() * size, uvNormalized.y() * size);

        if (uv.x() >= selectionAreaImage.width()  || uv.x() < 0 || uv.y() >= selectionAreaImage.height() || uv.y() < 0)
            continue;

        if (selectionAreaImage.pixelColor(uv).alpha() > 0)
	        targetSelectionIndices.push_back(localGlobalIndices[localPointIndex]);
    }

    switch (const auto selectionModifier = pixelSelectionTool.isAborted() ? PixelSelectionModifierType::Subtract : pixelSelectionTool.getModifier())
    {
        case PixelSelectionModifierType::Replace:
            break;

        case PixelSelectionModifierType::Add:
        case PixelSelectionModifierType::Subtract:
        {
            // Get reference to the indices of the selection set
            auto& selectionSetIndices = selectionSet->indices;

            // Create a set from the selection set indices
            QSet<std::uint32_t> set(selectionSetIndices.begin(), selectionSetIndices.end());

            switch (selectionModifier)
            {
                // Add points to the current selection
                case PixelSelectionModifierType::Add:
                {
                    // Add indices to the set 
                    for (const auto& targetIndex : targetSelectionIndices)
                        set.insert(targetIndex);

                    break;
                }

                // Remove points from the current selection
                case PixelSelectionModifierType::Subtract:
                {
                    // Remove indices from the set 
                    for (const auto& targetIndex : targetSelectionIndices)
                        set.remove(targetIndex);

                    break;
                }

                case PixelSelectionModifierType::Replace:
                    break;
            }

            targetSelectionIndices = std::vector<std::uint32_t>(set.begin(), set.end());

            break;
        }
    }

    _positionDataset->setSelectionIndices(targetSelectionIndices);

    events().notifyDatasetDataSelectionChanged(_positionDataset->getSourceDataset<Points>());
}

void TransferFunctionPlugin::samplePoints()
{
    auto& samplerPixelSelectionTool = _scatterPlotWidget->getSamplerPixelSelectionTool();

    if (!_positionDataset.isValid() || _scatterPlotWidget->isNavigating())
        return;

    auto selectionAreaImage = samplerPixelSelectionTool.getAreaPixmap().toImage();

    auto selectionSet = _positionDataset->getSelection<Points>();

    std::vector<std::uint32_t> targetSelectionIndices;

    targetSelectionIndices.reserve(_positionDataset->getNumPoints());

    std::vector<std::uint32_t> localGlobalIndices;

    _positionDataset->getGlobalIndices(localGlobalIndices);

    auto& zoomRectangleAction = _scatterPlotWidget->getNavigationAction().getZoomRectangleAction();

    const auto width    = selectionAreaImage.width();
    const auto height   = selectionAreaImage.height();
    const auto size     = width < height ? width : height;
    const auto uvOffset = QPoint((selectionAreaImage.width() - size) / 2.0f, (selectionAreaImage.height() - size) / 2.0f);

    QPointF pointUvNormalized;

    QPoint  pointUv, mouseUv = _scatterPlotWidget->mapFromGlobal(QCursor::pos());
    
    std::vector<char> focusHighlights(_positions.size());

    std::vector<std::pair<float, std::uint32_t>> sampledPoints;

    for (std::uint32_t localPointIndex = 0; localPointIndex < _positions.size(); localPointIndex++) {
        pointUvNormalized   = QPointF((_positions[localPointIndex].x - zoomRectangleAction.getLeft()) / zoomRectangleAction.getWidth(), (zoomRectangleAction.getTop() - _positions[localPointIndex].y) / zoomRectangleAction.getHeight());
        pointUv             = uvOffset + QPoint(pointUvNormalized.x() * size, pointUvNormalized.y() * size);

        if (pointUv.x() >= selectionAreaImage.width() || pointUv.x() < 0 || pointUv.y() >= selectionAreaImage.height() || pointUv.y() < 0)
            continue;

        if (selectionAreaImage.pixelColor(pointUv).alpha() > 0) {
            const auto sample = std::pair<float, std::uint32_t>((QVector2D(mouseUv) - QVector2D(pointUv)).length(), localPointIndex);

            sampledPoints.emplace_back(sample);
        }
    }
    
    QVariantList localPointIndices, globalPointIndices, distances;

    localPointIndices.reserve(static_cast<std::int32_t>(sampledPoints.size()));
    globalPointIndices.reserve(static_cast<std::int32_t>(sampledPoints.size()));
    distances.reserve(static_cast<std::int32_t>(sampledPoints.size()));

    std::int32_t numberOfPoints = 0;

    std::sort(sampledPoints.begin(), sampledPoints.end(), [](const auto& sampleA, const auto& sampleB) -> bool {
        return sampleB.first > sampleA.first;
    });

    for (const auto& sampledPoint : sampledPoints) {
        if (getSamplerAction().getRestrictNumberOfElementsAction().isChecked() && numberOfPoints >= getSamplerAction().getMaximumNumberOfElementsAction().getValue())
            break;

        const auto& distance            = sampledPoint.first;
        const auto& localPointIndex     = sampledPoint.second;
        const auto& globalPointIndex    = localGlobalIndices[localPointIndex];

        distances << distance;
        localPointIndices << localPointIndex;
        globalPointIndices << globalPointIndex;

        focusHighlights[localPointIndex] = true;

        numberOfPoints++;
    }

    if (getSamplerAction().getHighlightFocusedElementsAction().isChecked())
        const_cast<PointRenderer&>(_scatterPlotWidget->getPointRenderer()).setFocusHighlights(focusHighlights, static_cast<std::int32_t>(focusHighlights.size()));

    _scatterPlotWidget->update();


    getSamplerAction().setSampleContext({
        { "PositionDatasetID", _positionDataset.getDatasetId() },
        { "LocalPointIndices", localPointIndices },
        { "GlobalPointIndices", globalPointIndices },
        { "Distances", distances }
    });
}

Dataset<Points>& TransferFunctionPlugin::getPositionDataset()
{
    return _positionDataset;
}

Dataset<Points>& TransferFunctionPlugin::getPositionSourceDataset()
{
    return _positionSourceDataset;
}

void TransferFunctionPlugin::positionDatasetChanged()
{
    _dropWidget->setShowDropIndicator(!_positionDataset.isValid());
    _scatterPlotWidget->getPixelSelectionTool().setEnabled(_positionDataset.isValid());

    if (!_positionDataset.isValid())
        return;
     
    // Reset dataset references
    //_positionSourceDataset.reset();

    // Set position source dataset reference when the position dataset is derived
    //if (_positionDataset->isDerivedData())
    _positionSourceDataset = _positionDataset->getSourceDataset<Points>();

    _numPoints = _positionDataset->getNumPoints();
    
    updateData();
}

void TransferFunctionPlugin::loadColors(const Dataset<Points>& points, const std::uint32_t& dimensionIndex)
{
    // Only proceed with valid points dataset
    if (!points.isValid())
        return;

    // Generate point scalars for color mapping
    std::vector<float> scalars;

    if (_positionDataset->getNumPoints() != _numPoints)
    {
        qWarning("Number of points used for coloring does not match number of points in data, aborting attempt to color plot");
        return;
    }

    // Populate point scalars
    points->extractDataForDimension(scalars, dimensionIndex);

    // Assign scalars and scalar effect
    _scatterPlotWidget->setScalars(scalars);
    _scatterPlotWidget->setScalarEffect(PointEffect::Color);

    // Render
    getWidget().update();
}

TransferFunctionWidget& TransferFunctionPlugin::getTransferFunctionWidget()
{
    return *_scatterPlotWidget;
}

void TransferFunctionPlugin::updateData()
{
    // Check if the scatter plot is initialized, if not, don't do anything
    if (!_scatterPlotWidget->isInitialized())
        return;

    // If no dataset has been selected, don't do anything
    if (_positionDataset.isValid()) {



        // Determine number of points depending on if its a full dataset or a subset
        _numPoints = _positionDataset->getNumPoints();

        // Extract 2-dimensional points from the data set based on the selected dimensions
        _positionDataset->extractDataForDimensions(_positions, 0, 1);

        // Pass the 2D points to the scatter plot widget
        _scatterPlotWidget->setData(&_positions);

        updateSelection();
    }
    else {
        _numPoints = 0;
        _positions.clear();
        _scatterPlotWidget->setData(&_positions);
    }
}

void TransferFunctionPlugin::updateSelection()
{
    if (!_positionDataset.isValid())
        return;

    //Timer timer(__FUNCTION__);

    auto selection = _positionDataset->getSelection<Points>();

    std::vector<bool> selected;
    std::vector<char> highlights;

    _positionDataset->selectedLocalIndices(selection->indices, selected);

    highlights.resize(_positionDataset->getNumPoints(), 0);

    for (std::size_t i = 0; i < selected.size(); i++)
        highlights[i] = selected[i] ? 1 : 0;

    _scatterPlotWidget->setHighlights(highlights, static_cast<std::int32_t>(selection->indices.size()));

    if (getSamplerAction().getSamplingMode() == ViewPluginSamplerAction::SamplingMode::Selection) {
        std::vector<std::uint32_t> localGlobalIndices;

        _positionDataset->getGlobalIndices(localGlobalIndices);

        std::vector<std::uint32_t> sampledPoints;

        sampledPoints.reserve(_positions.size());

        for (auto selectionIndex : selection->indices)
            sampledPoints.push_back(selectionIndex);

        std::int32_t numberOfPoints = 0;

        QVariantList localPointIndices, globalPointIndices;

        const auto numberOfSelectedPoints = selection->indices.size();

        localPointIndices.reserve(static_cast<std::int32_t>(numberOfSelectedPoints));
        globalPointIndices.reserve(static_cast<std::int32_t>(numberOfSelectedPoints));

        for (const auto& sampledPoint : sampledPoints) {
            if (getSamplerAction().getRestrictNumberOfElementsAction().isChecked() && numberOfPoints >= getSamplerAction().getMaximumNumberOfElementsAction().getValue())
                break;

            const auto& localPointIndex = sampledPoint;
            const auto& globalPointIndex = localGlobalIndices[localPointIndex];

            localPointIndices << localPointIndex;
            globalPointIndices << globalPointIndex;

            numberOfPoints++;
        }

        _scatterPlotWidget->update();

        getSamplerAction().setSampleContext({
            { "PositionDatasetID", _positionDataset.getDatasetId() },
            { "LocalPointIndices", localPointIndices },
            { "GlobalPointIndices", globalPointIndices },
            { "Distances", QVariantList()}
		});
    }
}

void TransferFunctionPlugin::fromVariantMap(const QVariantMap& variantMap)
{
    ViewPlugin::fromVariantMap(variantMap);

    variantMapMustContain(variantMap, "Settings");

    _primaryToolbarAction.fromParentVariantMap(variantMap);
    _secondaryToolbarAction.fromParentVariantMap(variantMap);
    _settingsAction.fromParentVariantMap(variantMap);
    
    _scatterPlotWidget->getNavigationAction().fromParentVariantMap(variantMap);
}

QVariantMap TransferFunctionPlugin::toVariantMap() const
{
    QVariantMap variantMap = ViewPlugin::toVariantMap();

    _primaryToolbarAction.insertIntoVariantMap(variantMap);
    _secondaryToolbarAction.insertIntoVariantMap(variantMap);
    _settingsAction.insertIntoVariantMap(variantMap);

    _scatterPlotWidget->getNavigationAction().insertIntoVariantMap(variantMap);

    return variantMap;
}

std::uint32_t TransferFunctionPlugin::getNumberOfPoints() const
{
    if (!_positionDataset.isValid())
        return 0;

    return _positionDataset->getNumPoints();
}

void TransferFunctionPlugin::setXDimension(const std::int32_t& dimensionIndex)
{
    updateData();
}

void TransferFunctionPlugin::setYDimension(const std::int32_t& dimensionIndex)
{
    updateData();
}

TransferFunctionPluginFactory::TransferFunctionPluginFactory()
{
    getPluginMetadata().setDescription("TransferFunction view");
    getPluginMetadata().setSummary("High-performance transferFunction plugin for ManiVault Studio, capable of handling millions of data points.");
    getPluginMetadata().setCopyrightHolder({ "BioVault (Biomedical Visual Analytics Unit LUMC - TU Delft)" });
    getPluginMetadata().setAuthors({
        { "J. Thijssen", { "Software architect" }, { "LUMC", "TU Delft" } },
        { "T. Kroes", { "Lead software architect" }, { "LUMC" } },
        { "A. Vieth", { "Plugin developer", "Maintainer" }, { "LUMC", "TU Delft" } }
	});
    getPluginMetadata().setOrganizations({
        { "LUMC", "Leiden University Medical Center", "https://www.lumc.nl/en/" },
        { "TU Delft", "Delft university of technology", "https://www.tudelft.nl/" }
	});
    getPluginMetadata().setLicenseText("This plugin is distributed under the [LGPL v3.0](https://www.gnu.org/licenses/lgpl-3.0.en.html) license.");
}

QIcon TransferFunctionPluginFactory::getIcon(const QColor& color /*= Qt::black*/) const
{
    return Application::getIconFont("FontAwesome").getIcon("braille", color);
}

ViewPlugin* TransferFunctionPluginFactory::produce()
{
    return new TransferFunctionPlugin(this);
}

PluginTriggerActions TransferFunctionPluginFactory::getPluginTriggerActions(const mv::Datasets& datasets) const
{
    PluginTriggerActions pluginTriggerActions;

    const auto getInstance = [this]() -> TransferFunctionPlugin* {
        return dynamic_cast<TransferFunctionPlugin*>(Application::core()->getPluginManager().requestViewPlugin(getKind()));
    };

    const auto numberOfDatasets = datasets.count();

    if (PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
        auto& fontAwesome = Application::getIconFont("FontAwesome");

        if (numberOfDatasets >= 1) {
            auto pluginTriggerAction = new PluginTriggerAction(const_cast<TransferFunctionPluginFactory*>(this), this, "TransferFunction", "View selected datasets side-by-side in separate scatter plot viewers", fontAwesome.getIcon("braille"), [this, getInstance, datasets](PluginTriggerAction& pluginTriggerAction) -> void {
                for (const auto& dataset : datasets)
                    getInstance()->loadData(Datasets({ dataset }));
            });

            pluginTriggerActions << pluginTriggerAction;
        }
    }

    /*
    const auto numberOfPointsDatasets   = PluginFactory::getNumberOfDatasetsForType(datasets, PointType);
    const auto numberOfClusterDatasets  = PluginFactory::getNumberOfDatasetsForType(datasets, ClusterType);

    if (numberOfPointsDatasets == numberOfClusterDatasets) {
        QRegularExpression re("(Points, Clusters)");

        const auto reMatch = re.match(PluginFactory::getDatasetTypesAsStringList(datasets).join(","));

        if (reMatch.hasMatch() && reMatch.captured().count() == numberOfPointsDatasets) {
            auto pluginTriggerAction = createPluginTriggerAction("TransferFunction", "Load points dataset in separate viewer and apply cluster", datasets, "braille");

            connect(pluginTriggerAction, &QAction::triggered, [this, getInstance, datasets, numberOfPointsDatasets]() -> void {

                for (int i = 0; i < numberOfPointsDatasets; i++) {
                    getInstance()->loadData(Datasets({ datasets[i * 2] }));
                    getInstance()->loadColors(Dataset<Clusters>(datasets[i * 2 + 1]));
                }
            });

            pluginTriggerActions << pluginTriggerAction;
        }
    }
    */

    return pluginTriggerActions;
}

QUrl TransferFunctionPluginFactory::getRepositoryUrl() const
{
    return QUrl("https://github.com/ManiVaultStudio/TransferFunction");
}
