#include "MaterialTransitionsAction.h"
#include "TransferFunctionWidget.h"
#include "TransferFunctionPlugin.h"
#include "Application.h"

#include <QDebug>
#include <QHBoxLayout>
#include "InteractiveShape.h"

using namespace mv::gui;

MaterialTransitionsAction::MaterialTransitionsAction(QObject* parent, const QString& title) :
    WidgetAction(parent, title),
    _materialTransitionTable()
{
    setText(title);
    _materialTransitionTable.push_back(std::vector<QColor>());
    _materialTransitionTable[0].push_back(QColor(50, 50, 50, 0));
}

void MaterialTransitionsAction::initialize(TransferFunctionPlugin* transferFunctionPlugin)
{
    Q_ASSERT(transferFunctionPlugin != nullptr);

    if (transferFunctionPlugin == nullptr)
        return;

    TransferFunctionWidget& widget = transferFunctionPlugin->getTransferFunctionWidget();

    connect(&widget, &TransferFunctionWidget::shapeCreated, this, [this, &widget](InteractiveShape* shape) {
        // Resize the table to the new size
        auto& shapes = widget.getInteractiveShapes();
        int tableSize = shapes.size() + 1;
        _materialTransitionTable.resize(tableSize); 

        for (size_t i = 0; i < tableSize; ++i) {
            _materialTransitionTable[i].resize(tableSize); 

            // Fill the new transition cells with a transparent color
            if (i == tableSize - 1) {
                for (size_t j = 0; j < _materialTransitionTable[i].size(); ++j) {
                    _materialTransitionTable[i][j] = QColor(50, 50, 50, 0);
                }
            }
            else {
                _materialTransitionTable[i][tableSize - 1] = QColor(50, 50, 50, 0);
            }
        }

        emit transitionChanged(_materialTransitionTable);
        });

    connect(&widget, &TransferFunctionWidget::shapeDeleted, this, [this, &widget]() {
        // Resize the table to the new size
        auto& shapes = widget.getInteractiveShapes();
        int tableSize = shapes.size() + 1;
        _materialTransitionTable.resize(tableSize); 

        emit transitionChanged(_materialTransitionTable);
        });

    connect(this, &MaterialTransitionsAction::transitionChanged, &widget, [this, &widget](const std::vector<std::vector<QColor>>& transitions) {
		widget.updateMaterialTransitionTexture(transitions);
        });
}

void MaterialTransitionsAction::setTransitions(const std::vector<std::vector<QColor>>& transitions)
{
    _materialTransitionTable = transitions;
    emit transitionChanged(_materialTransitionTable);
}

void MaterialTransitionsAction::setSelectedTransition(const std::tuple<int, int>& selectedTransition)
{
	_selectedTransition = selectedTransition;
	emit transitionSelected(std::get<0>(_selectedTransition), std::get<1>(_selectedTransition));
}

void MaterialTransitionsAction::setColorOfCell(int row, int column, const QColor& color)
{
	_materialTransitionTable[row][column] = color;
	emit transitionChanged(_materialTransitionTable);
}

void MaterialTransitionsAction::connectToPublicAction(WidgetAction* publicAction, bool recursive)
{
    auto publicTransitionsAction = dynamic_cast<MaterialTransitionsAction*>(publicAction);

    Q_ASSERT(publicTransitionsAction != nullptr);

    if (publicTransitionsAction == nullptr)
        return;

    connect(this, &MaterialTransitionsAction::transitionChanged, publicTransitionsAction, &MaterialTransitionsAction::setTransitions);
    connect(publicTransitionsAction, &MaterialTransitionsAction::transitionChanged, this, &MaterialTransitionsAction::setTransitions);

    WidgetAction::connectToPublicAction(publicAction, recursive);
}

void MaterialTransitionsAction::disconnectFromPublicAction(bool recursive)
{
    if (!isConnected())
        return;

    auto publicTransitionsAction = dynamic_cast<MaterialTransitionsAction*>(getPublicAction());

    Q_ASSERT(publicTransitionsAction != nullptr);

    if (publicTransitionsAction == nullptr)
        return;

    disconnect(this, &MaterialTransitionsAction::transitionChanged, publicTransitionsAction, &MaterialTransitionsAction::setTransitions);
    disconnect(publicTransitionsAction, &MaterialTransitionsAction::transitionChanged, this, &MaterialTransitionsAction::setTransitions);

    WidgetAction::disconnectFromPublicAction(recursive);
}

MaterialTransitionsAction::Widget::Widget(QWidget* parent, MaterialTransitionsAction* materialTransitionsAction) :
    WidgetActionWidget(parent, materialTransitionsAction),
    _layout(),
    _tableWidget()
{
    setAcceptDrops(true);

	// Set the table to be non-editable and make sure that the background is still visible when selecting a cell
    _tableWidget.setEditTriggers(QAbstractItemView::NoEditTriggers);
    _tableWidget.setFocusPolicy(Qt::NoFocus);
    _tableWidget.setSelectionMode(QAbstractItemView::NoSelection);


    updateTable(materialTransitionsAction->_materialTransitionTable);

    _layout.addWidget(&_tableWidget);
    setLayout(&_layout);

    connect(materialTransitionsAction, &MaterialTransitionsAction::transitionChanged, this, [this, materialTransitionsAction](const std::vector<std::vector<QColor>>& transitions) {
        updateTable(transitions);
        });

	connect(&_tableWidget, &QTableWidget::cellClicked, this, [this, materialTransitionsAction](int row, int column) {
		materialTransitionsAction->setSelectedTransition(std::make_tuple(row, column));
		});
}

void MaterialTransitionsAction::Widget::updateTable(const std::vector<std::vector<QColor>>& transitions)
{
    int size = transitions.size();
    _tableWidget.setRowCount(size);
    _tableWidget.setColumnCount(size);

    for (int i = 0; i < size; ++i) {
        // Set the row height and column width to 10 
        _tableWidget.setRowHeight(i, 10);
        _tableWidget.setColumnWidth(i, 10);
        for (int j = 0; j < size; ++j) {
            QTableWidgetItem* item = new QTableWidgetItem();
            item->setBackground(transitions[i][j]);
            _tableWidget.setItem(i, j, item);
        }
    }
}

void MaterialTransitionsAction::fromVariantMap(const QVariantMap& variantMap)
{
    WidgetAction::fromVariantMap(variantMap);
}

QVariantMap MaterialTransitionsAction::toVariantMap() const
{
    QVariantMap variantMap = WidgetAction::toVariantMap();
    return variantMap;
}
