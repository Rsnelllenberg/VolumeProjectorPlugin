#pragma once

#include <renderers/Renderer.h>
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
#include <VolumeData/Volumes.h>
#include "VoxelBox.h"

namespace mv {
    class Texture3D : public Texture
    {
    public:
        Texture3D() : Texture(GL_TEXTURE_3D) {}

        void initialize() {
            bind();
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            release();
        }

        void setData(int width, int height, int depth, std::vector<float> textureData) {
            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, width, height, depth, 0, GL_RGBA, GL_FLOAT, textureData.data());
        }

    };
}

class VolumeRenderer : public mv::Renderer
{
public:
    void setData(const mv::Dataset<Volumes>& dataset, std::vector<std::uint32_t>& dimensionIndices);
    void setTransferfunction(const QImage& colormap);
    void setCamera(const TrackballCamera& camera);
    void setDefaultFramebuffer(GLuint defaultFramebuffer);
    void setClippingPlaneBoundery(mv::Vector3f min, mv::Vector3f max);
    mv::Vector3f getVolumeSize() { return _volumeSize; }

    void init() override;
    void resize(QSize renderSize) override;

    void renderDirections();

    void renderCompositeNoTF(mv::Texture3D& volumeTexture);

    void render() override;
    void updateMatrices();
    void drawDVRRender(mv::ShaderProgram& shader);
    void destroy() override;


private:
    mv::ShaderProgram _surfaceShader;
    mv::ShaderProgram _framebufferShader;
    mv::ShaderProgram _directionsShader;
    mv::ShaderProgram _noTFCompositeShader;

    int _numPoints = 0;
    mv::Vector3f _minClippingPlane;
    mv::Vector3f _maxClippingPlane;

    QOpenGLVertexArrayObject _vao;
    QOpenGLBuffer _vbo;
    QOpenGLBuffer _ibo = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);

    bool _hasColors = false;

    //QOpenGLTexture* _volumeTexture; //3D texture containing the volume data
    //GLuint _transferFunction;
    mv::Texture2D _frontfacesTexture;
    mv::Texture2D _directionsTexture;
    mv::Texture2D _depthTexture;
    mv::Texture2D _renderTexture;
    mv::Framebuffer _framebuffer;
    GLuint _defaultFramebuffer;

    QMatrix4x4 _modelMatrix;
    QMatrix4x4 _mvpMatrix;

    TrackballCamera _camera;
    mv::Texture3D _volumeTexture;
    mv::Dataset<Volumes> _volumeDataset;

    QSize _screenSize;
    mv::Vector3f _volumeSize = mv::Vector3f{50, 50, 50};
};

