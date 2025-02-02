#pragma once

#include "renderers/Renderer.h"
#include "renderers/PointRenderer.h"
#include "util/PixelSelectionTool.h"

#include "graphics/Bounds.h"
#include "graphics/Texture.h"
#include "graphics/Vector2f.h"
#include "graphics/Vector3f.h"

namespace mv {
    class TransferFunctionRenderer : public Renderer
    {
    public:
        void setData(const std::vector<Vector2f>& points);

		// Pass data to the point renderer
        void setBounds(const Bounds& bounds);
        void setViewBounds(const Bounds& boundsView);
        void setDataBounds(const Bounds& boundsData);
        void setPointSize(const float size);
        void setAlpha(const float alpha);
        void setPointScaling(gui::PointScaling scalingMode);
        void setPointColor(const Vector3f color);

		// Get data from the point renderer
        const gui::PointArrayObject& getGpuPoints() const;
        //std::int32_t getNumSelectedPoints() const;
        //const gui::PointSettings& getPointSettings() const;

		// Mandatory functions
        void init() override;
        void resize(QSize renderSize) override;
        void render() override;
        void destroy() override;

    signals:
        /** Signals that the selection shape changed */
        void shapeChanged();

        /** Signals that the selection process has started */
        void started();

        /** Signals that the selection process has ended */
        void ended();

    private:
        gui::PointRenderer _pointRenderer;
		util::PixelSelectionTool _pixelSelectionTool;

        QSize                       _windowSize;
        ShaderProgram               _shader;

		Texture2D				    _pointsTexture;                             /** Texture displaying all the points */
        Texture2D                   _materialAssignment;                        /** Texture that maps 2D position to a material */
    };
}


