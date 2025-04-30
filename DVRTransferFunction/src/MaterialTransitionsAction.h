#pragma once

#include "actions/WidgetAction.h"
#include "InteractiveShape.h"

#include <QVBoxLayout>
#include <QColorDialog>
#include <QTableWidget>

using namespace mv::gui;
class TransferFunctionPlugin;

class MaterialTransitionsAction : public WidgetAction
{
    Q_OBJECT
public:
    /**
        * Constructor
        * @param parent Pointer to parent object
        * @param title Title of the action
        */
    Q_INVOKABLE MaterialTransitionsAction(QObject* parent, const QString& title);

    void initialize(TransferFunctionPlugin* transferFunctionPlugin);
	std::vector<std::vector<QColor>> getTransitions() const { return _materialTransitionTable; }
	std::tuple<int, int> getSelectedTransition() const { return _selectedTransition; }

    void setColorOfCell(int row, int column, const QColor& color);
    void setUseGlobalAlpha(bool useGlobalAlpha);
    void setGlobalAlphaValue(int globalAlphaValue);

protected: // Linking
	void setTransitions(const std::vector<std::vector<QColor>>& transitions);
    void setSelectedTransition(const std::tuple<int, int>& selectedTransition);

    void connectToPublicAction(WidgetAction* publicAction, bool recursive) override;
    void disconnectFromPublicAction(bool recursive) override;

public: // Serialization

    void fromVariantMap(const QVariantMap& variantMap) override;
    QVariantMap toVariantMap() const override;

signals:
    void transitionChanged(const std::vector<std::vector<QColor>>& transitions);
	void headersChanged(std::vector<InteractiveShape> interactiveShapes);

    void transitionSelected(int row, int column);

	void globalAlphaToggled(bool useGlobalAlpha);
    void globalAlphaChanged(int globalAlphaValue);
	void tableUpdateNeeded(std::vector<InteractiveShape> interactiveShapes, std::vector<std::vector<QColor>> transitions);


protected:
    TransferFunctionPlugin*             _transferFunctionPlugin;        /** Pointer to scatterplot plugin */
    std::vector<std::vector<QColor>>    _materialTransitionTable;       /** Table of colors for the transitions */
	std::vector<InteractiveShape>       _interactiveShapes;			    /** Most recent list of interactive shapes */
    std::tuple<int, int>                _selectedTransition;            /** Currently selected transition */

	bool 							    _useGlobalAlpha = false;       /** The global alpha changes the alpha value of all the colors the user sees on their screen not the colors that are passes along */

    friend class mv::AbstractActionsManager;

    class Widget : public WidgetActionWidget {
    protected:
        Widget(QWidget* parent, MaterialTransitionsAction* materialTransitionsAction);

    protected:
        QVBoxLayout         _layout;                    /** Main layout */
        QTableWidget        _tableWidget;               /** Table widget to display colored cubes */

        bool                                _useGlobalAlpha = false;       /** The global alpha changes the alpha value of all the colors the user sees on their screen not the colors that are passes along */
        int                                 _globalAlphaValue = 100;

        void updateTable(const std::vector<std::vector<QColor>>& transitions);
        void updateHeaderColors(std::vector<InteractiveShape> interactiveShapes);

        friend class MaterialTransitionsAction;
    };

    QWidget* getWidget(QWidget* parent, const std::int32_t& widgetFlags) override {
        return new Widget(parent, this);
    };
};

Q_DECLARE_METATYPE(MaterialTransitionsAction)

inline const auto materialTransitionsActionMetaTypeId = qRegisterMetaType<MaterialTransitionsAction*>("MaterialTransitionsAction*");

