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
    _transferFunctionWidget(new TransferFunctionWidget()),
    _numPoints(0),
    _settingsAction(this, "Settings"),
    _primaryToolbarAction(this, "Primary Toolbar")
{
    setObjectName("TransferFunction");

    auto& shortcuts = getShortcuts();

    shortcuts.add({ QKeySequence(Qt::Key_R), "Selection", "Rectangle (default)" });
    shortcuts.add({ QKeySequence(Qt::Key_L), "Selection", "Lasso" });
    shortcuts.add({ QKeySequence(Qt::Key_B), "Selection", "Circular brush (mouse wheel adjusts the radius)" });
    shortcuts.add({ QKeySequence(Qt::SHIFT), "Selection", "Add to selection" });
    shortcuts.add({ QKeySequence(Qt::CTRL), "Selection", "Remove from selection" });

    shortcuts.add({ QKeySequence(Qt::Key_S), "Render", "Scatter mode (default)" });

    shortcuts.add({ QKeySequence(Qt::ALT), "Navigation", "Pan (LMB down)" });
    shortcuts.add({ QKeySequence(Qt::ALT), "Navigation", "Zoom (mouse wheel)" });
    shortcuts.add({ QKeySequence(Qt::Key_O), "Navigation", "Original view" });
    
    _dropWidget = new DropWidget(_transferFunctionWidget);

    _transferFunctionWidget->getNavigationAction().setParent(this);

    getWidget().setFocusPolicy(Qt::ClickFocus);

    _primaryToolbarAction.addAction(&_settingsAction.getDatasetsAction());

    _primaryToolbarAction.addAction(&_settingsAction.getSelectionAction());
	_primaryToolbarAction.addAction(&_settingsAction.getPointsAction());

    connect(_transferFunctionWidget, &TransferFunctionWidget::customContextMenuRequested, this, [this](const QPoint& point) {
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

                    dropRegions << new DropWidget::DropRegion(this, "Point position", description, "map-marker-alt", true, [this, candidateDataset]() {
                        _positionDataset = candidateDataset;
                        });
                }

                if (candidateDataset->getNumPoints() == _positionDataset->getNumPoints()) {

                }
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
    layout->addWidget(_transferFunctionWidget, 100);

    getWidget().setLayout(layout);

    // Update the data when the scatter plot widget is initialized
    connect(_transferFunctionWidget, &TransferFunctionWidget::initialized, this, &TransferFunctionPlugin::updateData);

    connect(&getSamplerAction(), &ViewPluginSamplerAction::sampleContextRequested, this, &TransferFunctionPlugin::samplePoints);

    connect(&_positionDataset, &Dataset<Points>::changed, this, &TransferFunctionPlugin::positionDatasetChanged);
    connect(&_positionDataset, &Dataset<Points>::dataChanged, this, &TransferFunctionPlugin::updateData);
    connect(&_positionDataset, &Dataset<Points>::dataSelectionChanged, this, &TransferFunctionPlugin::updateSelection);

    connect(&_settingsAction.getPointsAction().getSizeAction(), &DecimalAction::valueChanged, [this](float size) {
		_transferFunctionWidget->setPointSize(size);
        _transferFunctionWidget->update();
    });

	connect(&_settingsAction.getPointsAction().getOpacityAction(), &DecimalAction::valueChanged, [this](float opacity) {
		_transferFunctionWidget->setPointOpacity(opacity);
		_transferFunctionWidget->update();
		});

    _transferFunctionWidget->installEventFilter(this);

    getLearningCenterAction().getViewPluginOverlayWidget()->setTargetWidget(_transferFunctionWidget);
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

    // Avoid making a bigger subset than the current data by restricting the selection to the current data
    subset = _positionDataset->createSubsetFromVisibleSelection(name, _positionDataset);

    subset->getDataHierarchyItem().select();
}

void TransferFunctionPlugin::samplePoints()
{
    auto& samplerPixelSelectionTool = _transferFunctionWidget->getSamplerPixelSelectionTool();

    if (!_positionDataset.isValid() || _transferFunctionWidget->isNavigating())
        return;

    auto selectionAreaImage = samplerPixelSelectionTool.getAreaPixmap().toImage();

    auto selectionSet = _positionDataset->getSelection<Points>();

    std::vector<std::uint32_t> targetSelectionIndices;

    targetSelectionIndices.reserve(_positionDataset->getNumPoints());

    std::vector<std::uint32_t> localGlobalIndices;

    _positionDataset->getGlobalIndices(localGlobalIndices);

    auto& zoomRectangleAction = _transferFunctionWidget->getNavigationAction().getZoomRectangleAction();

    const auto width    = selectionAreaImage.width();
    const auto height   = selectionAreaImage.height();
    const auto size     = width < height ? width : height;
    const auto uvOffset = QPoint((selectionAreaImage.width() - size) / 2.0f, (selectionAreaImage.height() - size) / 2.0f);

    QPointF pointUvNormalized;

    QPoint  pointUv, mouseUv = _transferFunctionWidget->mapFromGlobal(QCursor::pos());
    
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
        const_cast<PointRenderer&>(_transferFunctionWidget->getPointRenderer()).setFocusHighlights(focusHighlights, static_cast<std::int32_t>(focusHighlights.size()));

    _transferFunctionWidget->update();


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

void TransferFunctionPlugin::positionDatasetChanged()
{
    _dropWidget->setShowDropIndicator(!_positionDataset.isValid());
    _transferFunctionWidget->getPixelSelectionTool().setEnabled(_positionDataset.isValid());

    if (!_positionDataset.isValid())
        return;

    _numPoints = _positionDataset->getNumPoints();
    
    updateData();
}

TransferFunctionWidget& TransferFunctionPlugin::getTransferFunctionWidget()
{
    return *_transferFunctionWidget;
}

void TransferFunctionPlugin::updateData()
{
    // Check if the scatter plot is initialized, if not, don't do anything
    if (!_transferFunctionWidget->isInitialized())
        return;

    // If no dataset has been selected, don't do anything
    if (_positionDataset.isValid()) {



        // Determine number of points depending on if its a full dataset or a subset
        _numPoints = _positionDataset->getNumPoints();

        // Extract 2-dimensional points from the data set based on the selected dimensions
        _positionDataset->extractDataForDimensions(_positions, 0, 1);

        // Pass the 2D points to the scatter plot widget
        _transferFunctionWidget->setData(&_positions);

        updateSelection();
    }
    else {
        _numPoints = 0;
        _positions.clear();
        _transferFunctionWidget->setData(&_positions);
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

    _transferFunctionWidget->setHighlights(highlights, static_cast<std::int32_t>(selection->indices.size()));

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

        _transferFunctionWidget->update();

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
    _settingsAction.fromParentVariantMap(variantMap);
    
    _transferFunctionWidget->getNavigationAction().fromParentVariantMap(variantMap);
}

QVariantMap TransferFunctionPlugin::toVariantMap() const
{
    QVariantMap variantMap = ViewPlugin::toVariantMap();

    _primaryToolbarAction.insertIntoVariantMap(variantMap);
    _settingsAction.insertIntoVariantMap(variantMap);

    _transferFunctionWidget->getNavigationAction().insertIntoVariantMap(variantMap);

    return variantMap;
}

std::uint32_t TransferFunctionPlugin::getNumberOfPoints() const
{
    if (!_positionDataset.isValid())
        return 0;

    return _positionDataset->getNumPoints();
}

TransferFunctionPluginFactory::TransferFunctionPluginFactory()
{
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

    return pluginTriggerActions;
}

QUrl TransferFunctionPluginFactory::getRepositoryUrl() const
{
    return QUrl("https://github.com/ManiVaultStudio/TransferFunction");
}
