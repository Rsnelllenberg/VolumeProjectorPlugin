#include "TransferFunctionRenderer.h"

namespace mv {
	void TransferFunctionRenderer::init()
	{
		// Initialize the point renderer
		_pointRenderer.init();

		_pixelSelectionTool.setEnabled(true);
		_pixelSelectionTool.setMainColor(QColor(Qt::black));
		_pixelSelectionTool.setFixedBrushRadiusModifier(Qt::AltModifier);

	}
	void TransferFunctionRenderer::resize(QSize renderSize)
	{
		int w = renderSize.width();
		int h = renderSize.height();

		_windowSize.setWidth(w);
		_windowSize.setHeight(h);

		// Resize the point renderer
		_pointRenderer.resize(renderSize);

		_shader.bind();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, renderSize.width(), renderSize.height(), 0, GL_RGBA, GL_FLOAT, nullptr);
		_shader.release();

		glViewport(0, 0, renderSize.width(), renderSize.height());
	}
	void TransferFunctionRenderer::setBounds(const Bounds& bounds)
	{
		setViewBounds(bounds);
		setDataBounds(bounds);
	}
	void TransferFunctionRenderer::setViewBounds(const Bounds& boundsView)
	{
		_pointRenderer.setViewBounds(boundsView);
	}
	void TransferFunctionRenderer::setDataBounds(const Bounds& boundsData)
	{
		_pointRenderer.setDataBounds(boundsData);
	}
	void TransferFunctionRenderer::setPointSize(const float size)
	{
		_pointRenderer.setPointSize(size);
	}
	void TransferFunctionRenderer::setAlpha(const float alpha)
	{
		_pointRenderer.setAlpha(alpha);
	}
	void TransferFunctionRenderer::setPointScaling(gui::PointScaling scalingMode)
	{
		_pointRenderer.setPointScaling(scalingMode);
	}
	void TransferFunctionRenderer::setPointColor(const Vector3f color)
	{
		int pointAmount = _pointRenderer.getGpuPoints().getPositions().size();
		_pointRenderer.setColors(std::vector<Vector3f>(pointAmount ,color));
	}
	const gui::PointArrayObject& TransferFunctionRenderer::getGpuPoints() const
	{
		_pointRenderer.getGpuPoints();
	}
}