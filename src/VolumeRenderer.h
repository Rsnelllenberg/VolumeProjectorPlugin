#pragma once


#include <renderers/Renderer.h>

#include "VoxelBox.h"
#include "TrackballCamera.h"
#include "graphics/Shader.h"
#include "graphics/Vector3f.h"
#include "graphics/Vector2f.h"
#include "graphics/Framebuffer.h"
#include "graphics/Texture.h"
#include <util/FileUtil.h>

#include <QMatrix4x4>

#include <vector>

/**
 * OpenGL Volume Renderer
 * This class provides a pure OpenGL renderer for volume data
 *
 * @author Julian Thijssen
 */

class VolumeRenderer : public mv::Renderer
{
public:
    void setData(std::vector<mv::Vector3f>& spatialData, std::vector<std::vector<float>>& valueData);
    void setTransferfunction(const QImage& colormap);
    void setCamera(const TrackballCamera& camera);
    void reloadShader();

    void init() override;
    void resize(QSize renderSize) override;

    void render() override;
    void destroy() override;

    const VoxelBox& getVoxelBox() const; // Add this line

private:
    //mv::Framebuffer _framebuffer;
    mv::ShaderProgram _volumeShaderProgram;
    mv::ShaderProgram _framebufferShaderProgram;

    int _numPoints = 0;

    GLuint _vao, _vbo, _ibo;

    bool _hasColors = false;

    //mv::Texture2D _generatedFrame;
    mv::Texture2D _transferFunction;
    GLuint _volumeTexture;

    QMatrix4x4 _projMatrix;
    QMatrix4x4 _viewMatrix;
    QMatrix4x4 _modelMatrix;

    TrackballCamera _camera;
    VoxelBox _voxelBox = VoxelBox(50, 50, 50, Bounds3D(-10, 10, -10, 10, -10, 10));
}; 

