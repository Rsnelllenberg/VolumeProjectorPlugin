#include "DVRTransferFunction.h"
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

Q_PLUGIN_METADATA(IID "studio.manivault.DVRTransferFunction")

using namespace mv::util;

// -----------------------------------------------------------------------------
// DVRViewPlugin
// -----------------------------------------------------------------------------
DVRTransferFunction::DVRTransferFunction(const PluginFactory* factory) :
    ViewPlugin(factory),
    _dropWidget(nullptr),
    _TFWidget(new TransferFunctionWidget()),
    _numPoints(0),
    _settingsAction(this, "Settings"),
	_primaryToolbarAction(this, "Primary Toolbar")

{
    qDebug() << "DVRTransferFunction0";
    setObjectName("DVR OpenGL view");

    getWidget().setFocusPolicy(Qt::ClickFocus);

    _primaryToolbarAction.addAction(&_settingsAction.getDatasetsAction());
    _primaryToolbarAction.addAction(&_settingsAction.getSelectionAction());
    _primaryToolbarAction.addAction(&getSamplerAction());

    qDebug() << "DVRTransferFunction1";
    // Instantiate new drop widget, setting the DVR Widget as its parent
    // the parent widget hat to setAcceptDrops(true) for the drop widget to work
    _dropWidget = new DropWidget(_TFWidget);

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
        const auto dataTypes = DataTypes({ PointType });

        if (dataTypes.contains(dataType)) {

            if (datasetId == getCurrentDataSetID()) {
                dropRegions << new DropWidget::DropRegion(this, "Warning", "Data already loaded", "exclamation-circle", false);
            }
            else {
                auto candidateDataset = mv::data().getDataset<Points>(datasetId);
				if (candidateDataset->getNumDimensions() != 2) {
					dropRegions << new DropWidget::DropRegion(this, "Incompatible data", "This type of data is not supported it only supports 2D Point Data", "exclamation-circle", false);
                }
                else {
                    dropRegions << new DropWidget::DropRegion(this, "Volumes", QString("Visualize %1 as parallel coordinates").arg(datasetGuiName), "map-marker-alt", true, [this, candidateDataset]() {
                        loadData({ candidateDataset });
                        });
                }
            }
        }
        else {
            dropRegions << new DropWidget::DropRegion(this, "Incompatible data", "This type of data is not supported it only supports Volume Data", "exclamation-circle", false);
        }

        return dropRegions;
    });
	qDebug() << "DVRTransferFunction2";
    auto& selectionAction = _settingsAction.getSelectionAction();

    getSamplerAction().initialize(this, &selectionAction.getPixelSelectionAction(), &selectionAction.getSamplerPixelSelectionAction());
    qDebug() << "DVRTransferFunction3";
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
    qDebug() << "DVRTransferFunction4";
    getSamplerAction().getEnabledAction().setChecked(false);

	qDebug() << "DVRTransferFunction created";

}

DVRTransferFunction::~DVRTransferFunction()
{
}

void DVRTransferFunction::init()
{
    auto layout = new QVBoxLayout();

    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(_primaryToolbarAction.createWidget(&getWidget()));
    layout->addWidget(_TFWidget, 100);

    getWidget().setLayout(layout);

    // Update the data when the scatter plot widget is initialized
    connect(_TFWidget, &TransferFunctionWidget::initialized, this, &DVRTransferFunction::updateData);

    connect(&_TFWidget->getPixelSelectionTool(), &PixelSelectionTool::areaChanged, [this]() {
        if (_TFWidget->getPixelSelectionTool().isNotifyDuringSelection()) {
            selectPoints();
        }
        });

    connect(&_TFWidget->getPixelSelectionTool(), &PixelSelectionTool::ended, [this]() {
        if (_TFWidget->getPixelSelectionTool().isNotifyDuringSelection())
            return;

        selectPoints();
        });

    connect(&getSamplerAction(), &ViewPluginSamplerAction::sampleContextRequested, this, &DVRTransferFunction::samplePoints);

    connect(&_pointsDataset, &Dataset<Points>::changed, this, &DVRTransferFunction::updatePlot);
    connect(&_pointsDataset, &Dataset<Points>::dataChanged, this, &DVRTransferFunction::updateData);
    connect(&_pointsDataset, &Dataset<Points>::dataSelectionChanged, this, &DVRTransferFunction::updateSelection);

    _TFWidget->installEventFilter(this);

    addDockingAction(&_settingsAction);
    _TFWidget->installEventFilter(this);

    connect(_TFWidget, &TransferFunctionWidget::initialized, this, []() { qDebug() << "DVRWidget is initialized."; } );
	qDebug() << "DVRTransferFunction initialized"; 
}

void DVRTransferFunction::updatePlot()
{
    _dropWidget->setShowDropIndicator(!_pointsDataset.isValid());
    _TFWidget->getPixelSelectionTool().setEnabled(_pointsDataset.isValid());

    if (!_pointsDataset.isValid())
        return;

    _numPoints = _pointsDataset->getNumPoints();

    updateData();
}

void DVRTransferFunction::updateData()
{
    // Check if the scatter plot is initialized, if not, don't do anything
    if (!_TFWidget->isInitialized())
        return;

    // If no dataset has been selected, don't do anything
    if (_pointsDataset.isValid()) {

        // Determine number of points depending on if its a full dataset or a subset
        _numPoints = _pointsDataset->getNumPoints();

		// We assume there are exactly two dimensions in the dataset and this gets the entire dataset
        _pointsDataset->extractDataForDimensions(_positions, 0, 1);

        // Pass the 2D points to the scatter plot widget
        _TFWidget->setData(&_positions);

        updateSelection();
    }
    else {
        _numPoints = 0;
        _positions.clear();
        _TFWidget->setData(&_positions);
    }
    _TFWidget->update();
}

void DVRTransferFunction::updateSelection()
{
    if (!_pointsDataset.isValid())
        return;
    //The below is commented out for now since we don't need it just yet

    //Timer timer(__FUNCTION__);

    auto selection = _pointsDataset->getSelection<Points>();

    std::vector<bool> selected;
    std::vector<char> highlights;

    _pointsDataset->selectedLocalIndices(selection->indices, selected);

    if (getSamplerAction().getSamplingMode() == ViewPluginSamplerAction::SamplingMode::Selection) {
        std::vector<std::uint32_t> localGlobalIndices;

        _pointsDataset->getGlobalIndices(localGlobalIndices);

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

        _TFWidget->update();

        //TODO: assign selected points to differnt matarial groups

        getSamplerAction().setSampleContext({
            { "PositionDatasetID", _pointsDataset.getDatasetId() },
            { "LocalPointIndices", localPointIndices },
            { "GlobalPointIndices", globalPointIndices },
            { "Distances", QVariantList()}
            });
    }
}

bool DVRTransferFunction::isDataValid() const
{
    return _pointsDataset.isValid();
}


void DVRTransferFunction::loadData(const mv::Datasets datasets)
{
    // Exit if there is nothing to load
    if (datasets.isEmpty())
        return;

    // Load the first dataset
    _pointsDataset = datasets.first();
}

Dataset<Points>& DVRTransferFunction::getPositionDataset()
{
    return _pointsDataset;
}

void DVRTransferFunction::selectPoints()
{
    auto& pixelSelectionTool = _TFWidget->getPixelSelectionTool();

    // Only proceed with a valid points position dataset and when the pixel selection tool is active
    if (!_pointsDataset.isValid() || !pixelSelectionTool.isActive() || _TFWidget->isNavigating())
        return;

    auto selectionAreaImage = pixelSelectionTool.getAreaPixmap().toImage();
    auto selectionSet = _pointsDataset->getSelection<Points>();

    std::vector<std::uint32_t> targetSelectionIndices;

    targetSelectionIndices.reserve(_pointsDataset->getNumPoints());

    std::vector<std::uint32_t> localGlobalIndices;

    _pointsDataset->getGlobalIndices(localGlobalIndices);

    auto zoomRectangleAction = _TFWidget->getBounds(); // TODO: check if this is correct

    const auto width = selectionAreaImage.width();
    const auto height = selectionAreaImage.height();
    const auto size = width < height ? width : height;
    const auto uvOffset = QPoint((selectionAreaImage.width() - size) / 2.0f, (selectionAreaImage.height() - size) / 2.0f);

    QPointF uvNormalized = {};
    QPoint uv = {};

    for (std::uint32_t localPointIndex = 0; localPointIndex < _positions.size(); localPointIndex++) {
        uvNormalized = QPointF((_positions[localPointIndex].x - zoomRectangleAction.getLeft()) / zoomRectangleAction.getWidth(), (zoomRectangleAction.getTop() - _positions[localPointIndex].y) / zoomRectangleAction.getHeight());
        uv = uvOffset + QPoint(uvNormalized.x() * size, uvNormalized.y() * size);

        if (uv.x() >= selectionAreaImage.width() || uv.x() < 0 || uv.y() >= selectionAreaImage.height() || uv.y() < 0)
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

    _pointsDataset->setSelectionIndices(targetSelectionIndices);

    events().notifyDatasetDataSelectionChanged(_pointsDataset->getSourceDataset<Points>());
}

void DVRTransferFunction::samplePoints()
{
    auto& samplerPixelSelectionTool = _TFWidget->getSamplerPixelSelectionTool();

    if (!_pointsDataset.isValid() || _TFWidget->isNavigating())
        return;

    auto selectionAreaImage = samplerPixelSelectionTool.getAreaPixmap().toImage();

    auto selectionSet = _pointsDataset->getSelection<Points>();

    std::vector<std::uint32_t> targetSelectionIndices;

    targetSelectionIndices.reserve(_pointsDataset->getNumPoints());

    std::vector<std::uint32_t> localGlobalIndices;

    _pointsDataset->getGlobalIndices(localGlobalIndices);

    auto zoomRectangleAction = _TFWidget->getBounds();

    const auto width = selectionAreaImage.width();
    const auto height = selectionAreaImage.height();
    const auto size = width < height ? width : height;
    const auto uvOffset = QPoint((selectionAreaImage.width() - size) / 2.0f, (selectionAreaImage.height() - size) / 2.0f);

    QPointF pointUvNormalized;

    QPoint  pointUv, mouseUv = _TFWidget->mapFromGlobal(QCursor::pos());

    std::vector<char> focusHighlights(_positions.size());

    std::vector<std::pair<float, std::uint32_t>> sampledPoints;

    for (std::uint32_t localPointIndex = 0; localPointIndex < _positions.size(); localPointIndex++) {
        pointUvNormalized = QPointF((_positions[localPointIndex].x - zoomRectangleAction.getLeft()) / zoomRectangleAction.getWidth(), (zoomRectangleAction.getTop() - _positions[localPointIndex].y) / zoomRectangleAction.getHeight());
        pointUv = uvOffset + QPoint(pointUvNormalized.x() * size, pointUvNormalized.y() * size);

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

        const auto& distance = sampledPoint.first;
        const auto& localPointIndex = sampledPoint.second;
        const auto& globalPointIndex = localGlobalIndices[localPointIndex];

        distances << distance;
        localPointIndices << localPointIndex;
        globalPointIndices << globalPointIndex;

        focusHighlights[localPointIndex] = true;

        numberOfPoints++;
    }

    if (getSamplerAction().getHighlightFocusedElementsAction().isChecked())
        const_cast<PointRenderer&>(_TFWidget->getPointRenderer()).setFocusHighlights(focusHighlights, static_cast<std::int32_t>(focusHighlights.size()));

    _TFWidget->update();

    getSamplerAction().setSampleContext({
        { "PositionDatasetID", _pointsDataset.getDatasetId() },
        { "LocalPointIndices", localPointIndices },
        { "GlobalPointIndices", globalPointIndices },
        { "Distances", distances }
        });
}

QString DVRTransferFunction::getCurrentDataSetID() const
{
    if (_pointsDataset.isValid())
        return _pointsDataset->getId();
    else
        return QString{};
}

// -----------------------------------------------------------------------------
// DVRViewPluginFactory
// -----------------------------------------------------------------------------

DVRTransferFunctionFactory::DVRTransferFunctionFactory()
{

}

QIcon DVRTransferFunctionFactory::getIcon(const QColor& color /*= Qt::black*/) const
{
    return Application::getIconFont("FontAwesome").getIcon("braille", color);
}

ViewPlugin* DVRTransferFunctionFactory::produce()
{
    return new DVRTransferFunction(this);
}

PluginTriggerActions DVRTransferFunctionFactory::getPluginTriggerActions(const mv::Datasets& datasets) const
{
    PluginTriggerActions pluginTriggerActions;

    const auto getInstance = [this]() -> DVRTransferFunction* {
        return dynamic_cast<DVRTransferFunction*>(Application::core()->getPluginManager().requestViewPlugin(getKind()));
    };

    const auto numberOfDatasets = datasets.count();

    if (PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
        auto& fontAwesome = Application::getIconFont("FontAwesome");

        if (numberOfDatasets >= 1) {
            auto pluginTriggerAction = new PluginTriggerAction(const_cast<DVRTransferFunctionFactory*>(this), this, "Scatterplot", "View selected datasets side-by-side in separate scatter plot viewers", fontAwesome.getIcon("braille"), [this, getInstance, datasets](PluginTriggerAction& pluginTriggerAction) -> void {
                for (const auto& dataset : datasets)
                    getInstance()->loadData(Datasets({ dataset }));
            });

            pluginTriggerActions << pluginTriggerAction;
        }
    }

    return pluginTriggerActions;
}

QUrl DVRTransferFunctionFactory::getRepositoryUrl() const
{
    return QUrl("https://github.com/ManiVaultStudio/Scatterplot");
}
