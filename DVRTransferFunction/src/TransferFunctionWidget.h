#pragma once

#include <renderers/DensityRenderer.h>
#include <renderers/PointRenderer.h>

#include <util/PixelSelectionTool.h>

#include <actions/DecimalRectangleAction.h>

#include <graphics/Bounds.h>
#include <graphics/Vector2f.h>
#include <graphics/Vector3f.h>

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLWidget>
#include <QPoint>

#include "InteractiveShape.h"
#include "ImageData/Images.h"


class TransferFunctionPlugin;

using namespace mv::gui;
using namespace mv::util;

struct widgetSizeInfo {
    float width;
    float height;
    float minWH;
    float ratioWidth;
    float ratioHeight;
};

class TransferFunctionWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    TransferFunctionWidget();

    ~TransferFunctionWidget();

    QRect getMousePositionsBounds(QPoint newMousePosition);

    /** Returns true when the widget was initialized and is ready to be used. */
    bool isInitialized() const;

	// This method is public because the UI decides when to update the widget
    void updateMaterialTransitionTexture(std::vector<std::vector<QColor>> transitionsTable);

	InteractiveShape* getSelectedObject() { return _selectedObject; }
	std::vector<InteractiveShape>& getInteractiveShapes() { return _interactiveShapes; }

    /**
     * Get the pixel selection tool
     * @return Reference to the pixel selection tool
     */
    PixelSelectionTool& getPixelSelectionTool();

    /**
     * Feed 2-dimensional data to the transferFunction.
     */
    void setData(const std::vector<mv::Vector2f>* data);
    void setHighlights(const std::vector<char>& highlights, const std::int32_t& numSelectedPoints);

    /**
     * Set point size
     */
    void setPointSize(float pointSize);

    /**
     * Set point opacity 
     */
    void setPointOpacity(float pointOpacity);

    void showHighlights(bool show);

    mv::Bounds getBounds() const {
        return _dataRectangleAction.getBounds();
    }

public: // Selection

    /**
     * Get the selection display mode color
     * @return Selection display mode
     */
    PointSelectionDisplayMode getSelectionDisplayMode() const;

    /**
     * Set the selection display mode
     * @param selectionDisplayMode Selection display mode
     */
    void setSelectionDisplayMode(PointSelectionDisplayMode selectionDisplayMode);

    /**
     * Set whether the selection outline halo is enabled or not
     * @param randomizedDepth Boolean determining whether the selection outline halo is enabled or not
     */
    void setRandomizedDepthEnabled(bool randomizedDepth);

    /**
     * Set whether the z-order of each point is to be randomized or not
     * @param selectionOutlineHaloEnabled Boolean determining whether the z-order of each point is to be randomized or not
     */
    void setSelectionOutlineHaloEnabled(bool selectionOutlineHaloEnabled);

    /**
     * Get whether the z-order of each point is to be randomized or not
     * @return Boolean determining whether the z-order of each point is to be randomized or not
     */
    bool getRandomizedDepthEnabled() const;

protected:
    void initializeGL()         Q_DECL_OVERRIDE;
    void resizeGL(int w, int h) Q_DECL_OVERRIDE;
    void paintGL()              Q_DECL_OVERRIDE;
    void paintPixelSelectionToolNative(PixelSelectionTool& pixelSelectionTool, QImage& image) const;
	void updateTfTexture();
	void updateMaterialPositionsTexture();

    void cleanup();
    
    void showEvent(QShowEvent* event) Q_DECL_OVERRIDE
    {
        emit created();
        QWidget::showEvent(event);
    }

    bool event(QEvent* event) override;

public: // Const access to renderers

    const PointRenderer& getPointRenderer() const {
        return _pointRenderer;
    }

public:

    /** Assign a color map image to the point and density renderers */
    void setColorMap(const QImage& colorMapImage);

signals:
    void initialized();
    void created();

	// Signals when a new shape gets selected and passes a pointer of it
	void shapeSelected(InteractiveShape* shape);
	void shapeCreated(InteractiveShape* shape);
	void shapeDeleted();
    
private slots:
    void updatePixelRatio();

private:
    mv::Dataset<Images> 			_tfTextures;                        /** Smart pointer to images dataset for point color */
	mv::Dataset<Points>             _tfSourceDataset;                   /** Smart pointer to points dataset for point position */
	mv::Dataset<Images>             _materialTransitionTexture; 	    /** Smart pointer to images dataset for material transition texture */
	mv::Dataset<Points>             _materialTransitionSourceDataset;   /** Smart pointer to points dataset for material transition points */
	mv::Dataset<Images>             _materialPositionTexture;           /** Smart pointer to images dataset for material positions texture */
	mv::Dataset<Points>             _materialPositionSourceDataset;     /** Smart pointer to points dataset for material positions points */

    PointRenderer                   _pointRenderer;                     /** For rendering point data as points */
    bool                            _isInitialized;                     /** Boolean determining whether the widget it properly initialized or not */
    QColor                          _backgroundColor;                   /** Background color */
    widgetSizeInfo                  _widgetSizeInfo;                    /** Info about size of the transferFunction widget */
    DecimalRectangleAction          _dataRectangleAction;               /** Rectangle action for the bounds of the loaded data */
    PixelSelectionTool              _pixelSelectionTool;                /** 2D pixel selection tool */
    float                           _pixelRatio;                        /** Current pixel ratio */
    QVector<QPoint>                 _mousePositions;                    /** Recorded mouse positions */
    bool                            _mouseIsPressed;                    /** Boolean determining the mouse state */
	QRect 						    _areaSelectionBounds;               /** Area selection bounds */
	QRect 						    _boundsPointsWindow; 		        /** Bounds of the points in the UI window */

	std::vector<InteractiveShape>   _interactiveShapes;                 /** Stores all the interactive shapes in the transferfunction widget*/
	InteractiveShape*               _selectedObject = nullptr;	        /** Pointer to the selected object */
	SelectedSide					_selectedSide = SelectedSide::None; /** Selected side of the object */
	bool 						    _createShape = false;               /** Boolean determining whether a shape is to be created or not */

    const int _tfTextureSize = 512;
    const int _materialTextureSize = 128;
	const int _materialPositionTextureSize = 512;
};
