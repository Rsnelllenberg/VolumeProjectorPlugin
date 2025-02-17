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
#include <ImageData/Images.h>
#include <PointData/PointData.h>

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

        void setData(int width, int height, int depth, std::vector<float> textureData, int voxelDimensions) {
            if(voxelDimensions == 1)
                glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, width, height, depth, 0, GL_RED, GL_FLOAT, textureData.data());
            else if (voxelDimensions == 2)
                glTexImage3D(GL_TEXTURE_3D, 0, GL_RG32F, width, height, depth, 0, GL_RG, GL_FLOAT, textureData.data());
            else if (voxelDimensions == 3)
                glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB32F, width, height, depth, 0, GL_RGB, GL_FLOAT, textureData.data());
            else if (voxelDimensions == 4)
                glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, width, height, depth, 0, GL_RGBA, GL_FLOAT, textureData.data());
            else
                qCritical() << "Unsupported voxel dimensions";
        }

    };
}

enum RenderMode {
    MULTIDIMENSIONAL_COMPOSITE_FULL,
    MULTIDIMENSIONAL_COMPOSITE_2D_POS,
    MULTIDIMENSIONAL_COMPOSITE_COLOR,
    MIP,
    MaterialTransition_2D,
    MaterialTransition_FULL
};

class VolumeRenderer : public mv::Renderer
{
public:
    void setData(const mv::Dataset<Volumes>& dataset);
    void setTfTexture(const mv::Dataset<Images>& tfTexture);
    void setReducedPosData(const mv::Dataset<Points>& reducedPosData);
    void setMaterialTransitionTexture(const mv::Dataset<Images>& materialTransitionTexture);

    void setCamera(const TrackballCamera& camera);
    void setDefaultFramebuffer(GLuint defaultFramebuffer);
    void setClippingPlaneBoundery(mv::Vector3f min, mv::Vector3f max);
    void setRenderSpace(mv::Vector3f size);
    void setUseCustomRenderSpace(bool useCustomRenderSpace);
    void setCompositeIndices(std::vector<std::uint32_t> compositeIndices);

    void setRenderMode(const QString& renderMode);
    void setMIPDimension(int mipDimension);

    void updataDataTexture();

    mv::Vector3f getVolumeSize() { return _volumeSize; }

    void init() override;
    void resize(QSize renderSize) override;



    void setDefaultRenderSettings();

    void render() override;
    void destroy() override;

private:
    void renderDirections();
    void renderDirectionsTexture();
    void updateMatrices();
    void drawDVRRender(mv::ShaderProgram& shader);

    void renderCompositeFull();
    void renderComposite2DPos();
    void renderCompositeColor();
    void render1DMip();

    void normalizePositionData(std::vector<float>& positionData);

private:
    RenderMode                  _renderMode;          /* Render mode options*/
    int                         _mipDimension;
    std::vector<std::uint32_t>  _compositeIndices;

    mv::ShaderProgram _surfaceShader;
    mv::ShaderProgram _framebufferShader;
    mv::ShaderProgram _directionsShader;
    mv::ShaderProgram _2DCompositeShader;
    mv::ShaderProgram _colorCompositeShader;
    mv::ShaderProgram _1DMipShader;

    int _numPoints = 0;
    mv::Vector3f _minClippingPlane;
    mv::Vector3f _maxClippingPlane;

    QOpenGLVertexArrayObject _vao;
    QOpenGLBuffer _vbo;
    QOpenGLBuffer _ibo = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);

    bool _hasColors = false;
    bool _settingsChanged = true;
    bool _useCustomRenderSpace = false;

    mv::Texture2D _frontfacesTexture;
    mv::Texture2D _directionsTexture;
    mv::Texture2D _depthTexture;
    mv::Texture2D _renderTexture;
    mv::Texture2D _tfTexture;                   //2D texture containing the transfer function
    mv::Texture2D _materialTransitionTexture;   //2D texture containing the material transition texture
    mv::Texture3D _volumeTexture;               //3D texture containing the volume data

    mv::Framebuffer _framebuffer;
    GLuint _defaultFramebuffer;

    QMatrix4x4 _modelMatrix;
    QMatrix4x4 _mvpMatrix;

    TrackballCamera _camera;
    mv::Dataset<Volumes> _volumeDataset;
    mv::Dataset<Images> _tfDataset;
    mv::Dataset<Points> _reducedPosDataset;
    mv::Dataset<Images> _materialTransitionDataset;

    QSize _screenSize;
    mv::Vector3f _volumeSize = mv::Vector3f{50, 50, 50};
    mv::Vector3f _renderSpace = mv::Vector3f{ 50, 50, 50 };
    QPair<float, float> _scalarVolumeDataRange;
    QPair<float, float> _scalarImageDataRange;
    QVector<float> _imageData;
};

