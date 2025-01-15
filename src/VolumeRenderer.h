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
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <vector>

/**
 * OpenGL Volume Renderer
 * This class provides a pure OpenGL renderer for volume data
 *
 * @autor Julian Thijssen
 */

class VolumeRenderer : public mv::Renderer
{
public:
    void setData(const std::vector<mv::Vector3f>& spatialData, const std::vector<std::vector<float>>& valueData);
    void setTransferfunction(const QImage& colormap);
    void setCamera(const TrackballCamera& camera);
    void reloadShader();

    void init() override;
    void resize(QSize renderSize) override;

    void renderDirections();

    void render() override;
    void updateMatrices();
    void drawDVRRender(mv::ShaderProgram& shader);
    void destroy() override;

    const VoxelBox& getVoxelBox() const;

private:
    mv::ShaderProgram _surfaceShader;
    mv::ShaderProgram _framebufferShader;
    mv::ShaderProgram _directionsShader;

    int _numPoints = 0;

    QOpenGLVertexArrayObject _vao;
    QOpenGLBuffer _vbo;
    QOpenGLBuffer _ibo;

    bool _hasColors = false;

    //QOpenGLTexture* _volumeTexture; //3D texture containing the volume data
    //GLuint _transferFunction;
    GLuint _frontfacesTexture;
    GLuint _directionsTexture;
    GLuint _depthTexture;
    GLuint _fbo;

    QMatrix4x4 _modelMatrix;
    QMatrix4x4 _mvpMatrix;

    TrackballCamera _camera;
    VoxelBox _voxelBox = VoxelBox(50, 50, 50, Bounds3D(0, 10, 0, 10, 0, 10));

    QSize _screenSize;
};

