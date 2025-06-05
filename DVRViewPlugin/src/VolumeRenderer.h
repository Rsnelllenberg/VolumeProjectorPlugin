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
#include "MCArrays.h"

#include <hnswlib.h>
#include <QOpenGLFunctions_4_3_Core>

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

        void setData(int width, int height, int depth, std::vector<float> _textureData, int voxelDimensions) {
            if(voxelDimensions == 1)
                glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, width, height, depth, 0, GL_RED, GL_FLOAT, _textureData.data());
            else if (voxelDimensions == 2)
                glTexImage3D(GL_TEXTURE_3D, 0, GL_RG32F, width, height, depth, 0, GL_RG, GL_FLOAT, _textureData.data());
            else if (voxelDimensions == 3)
                glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB32F, width, height, depth, 0, GL_RGB, GL_FLOAT, _textureData.data());
            else if (voxelDimensions == 4)
                glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, width, height, depth, 0, GL_RGBA, GL_FLOAT, _textureData.data());
            else
                qCritical() << "Unsupported voxel dimensions";
        }

    };
}

enum RenderMode {
    MULTIDIMENSIONAL_COMPOSITE_FULL,
    MULTIDIMENSIONAL_COMPOSITE_2D_POS,
    MULTIDIMENSIONAL_COMPOSITE_COLOR,
    NN_MULTIDIMENSIONAL_COMPOSITE,
    MIP,
    NN_MaterialTransition,
    Smooth_NN_MaterialTransition,
    Alt_NN_MaterialTransition,
    MaterialTransition_2D,
    MaterialTransition_FULL
};

class VolumeRenderer : protected QOpenGLFunctions_4_3_Core
{
public:
    void setData(const mv::Dataset<Volumes>& dataset);
    void setTfTexture(const mv::Dataset<Images>& tfTexture);
    void setReducedPosData(const mv::Dataset<Points>& reducedPosData);
    void setMaterialTransitionTexture(const mv::Dataset<Images>& materialTransitionTexture);
    void setMaterialPositionTexture(const mv::Dataset<Images>& materialPositionTexture);

    void setCamera(const TrackballCamera& camera);
    void setDefaultFramebuffer(GLuint defaultFramebuffer);
    void setClippingPlaneBoundery(mv::Vector3f min, mv::Vector3f max);
    void setStepSize(float stepSize);
    void setRenderSpace(mv::Vector3f size);
    void setUseCustomRenderSpace(bool useCustomRenderSpace);
    void setCompositeIndices(std::vector<std::uint32_t> compositeIndices);

    void setRenderMode(const QString& renderMode);
    void setMIPDimension(int mipDimension);
    void setUseShading(bool useShading);
    void setUseEmptySpaceSkipping(bool useEmptySpaceSkipping);

    void setRenderCubeSize(float renderCubeSize);

    void updataDataTexture();

    mv::Vector3f getVolumeSize() { return _volumeSize; }
    bool getFullRenderModeInProgress() { return _fullDataModeBatch != -1; }

    void init();
    void resize(QSize renderSize);

    void setDefaultRenderSettings();

    void render();
    void destroy();

private:
    void renderDirections();
    void renderTexture(mv::Texture2D& texture);
    void updateMatrices();

    void drawDVRRender(mv::ShaderProgram& shader);
    void drawDVRQuad(mv::ShaderProgram& shader);

    // Full data render mode methods
    void prepareHNSW();
    std::vector<std::vector<std::pair<float, hnswlib::labeltype>>> batchSearch(const std::vector<float>& queryData, const std::vector<int>& maskData, uint32_t dimensions, int k);
    void getFacesTextureData(std::vector<float>& frontfacesData, std::vector<float>& backfacesData);
    void getGPUFullDataModeBatches(std::vector<float>& frontfacesData, std::vector<float>& backfacesData, std::vector<size_t>& _subsetsMemory, std::vector<std::vector<int>>& _GPUBatches, std::vector<std::vector<int>>& GPUBatchesReservedRayMemory);
    void retrieveBatchFullData(std::vector<float>& cpuOutput, std::vector<int>& maskOutput, std::vector<float>& samplePositions, std::vector<size_t> _subsetsMemory, int batchIndex, std::vector<std::vector<int>> _GPUBatches, std::vector<std::vector<int>> _GPUBatchesStartIndex, bool deleteBuffers);
    void renderBatchToScreen(std::vector<std::vector<int>>& _GPUBatchesStartIndex, int batchIndex, uint32_t sampleDim, std::vector<float>& meanPositions, std::vector<std::vector<int>>& _GPUBatches);
    void ComputeMeanOfNN(std::vector<std::vector<std::pair<float, hnswlib::labeltype>>>& nnResults, int k, std::vector<float>& positionData, bool useWeightedMean, std::vector<float>& meanPositions);
    void updateRenderModeParameters();

    void renderCompositeFull();
    void renderComposite2DPos();
    void renderCompositeColor();
    void render1DMip();

    void renderMaterialTransitionFull();
    void renderMaterialTransition2D();
    void renderNNMaterialTransition();
    void renderAltNNMaterialTransition();
    //void renderSmoothNNMaterialTransition();

    void normalizePositionData(std::vector<float>& positionData);
    void updateAuxilairySmoothNNTextures();
    //void updateRenderCubes();
    //void updateRenderCubes2DCoords();

private:
    RenderMode                  _renderMode;          /* Render mode options*/
    int                         _mipDimension;
    std::vector<std::uint32_t>  _compositeIndices;

    mv::ShaderProgram _surfaceShader;
    mv::ShaderProgram _textureShader;
    mv::ShaderProgram _2DCompositeShader;
    mv::ShaderProgram _colorCompositeShader;
    mv::ShaderProgram _1DMipShader;
    mv::ShaderProgram _materialTransition2DShader;
    mv::ShaderProgram _nnMaterialTransitionShader;
    mv::ShaderProgram _altNNMaterialTransitionShader;
    mv::ShaderProgram _fullDataCompositeShader;
    QOpenGLShaderProgram* _fullDataSamplerComputeShader; // This has a differnt type since mv::ShaderProgram does not support compute shaders

    mv::Vector3f _minClippingPlane;
    mv::Vector3f _maxClippingPlane;

    QOpenGLVertexArrayObject _vao;
    QOpenGLBuffer _vboCube;
    QOpenGLBuffer _iboCube = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    QOpenGLBuffer _vboQuad;
    QOpenGLBuffer _iboQuad = QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);

    bool _hasColors = false;
    bool _dataSettingsChanged = true;
    bool _useCustomRenderSpace = false;
    bool _useShading = false;
    bool _useEmptySpaceSkipping = false;
    bool _renderCubesUpdated = false;
    bool _ANNAlgorithmTrained = false; 

    int _renderCubeSize = 20;
    int _renderCubeAmount = 1;

    mv::Texture2D _frontfacesTexture;
    mv::Texture2D _backfacesTexture;
    mv::Texture2D _depthTexture;
    mv::Texture2D _prevFullCompositeTexture; // The previous screen texture, used for the full data mode

    mv::Texture2D _tfTexture;                   //2D texture containing the transfer function
    mv::Texture2D _tfRectangleDataTexture;      //2D texture containing the transfer function bounding rectangles
    mv::Texture2D _materialTransitionTexture;   //2D texture containing the material transition texture
    mv::Texture2D _materialPositionTexture;     //2D texture containing the material position texture
    mv::Texture3D _volumeTexture;               //3D texture containing the volume data
    mv::Texture3D _volumeMaskFullDataTexture;   //3D texture containing the a mask for the volume data for the full data mode

    // IDs for the render cube buffers
    GLuint _renderCubePositionsBufferID;
    GLuint _renderCubePositionsTexID;
    GLuint _renderCubeOccupancyBufferID; // for 2D occupancy it is ordered as topleft(x,y), bottomright(x,y)
    GLuint _renderCubeOccupancyTexID;
    // Create and bind the Marching cubes SSBOs
    GLuint edgeTableSSBO, triTableSSBO;

    //Large GPU buffers for the full data mode
    GLuint _indicesSSBO;
    GLuint _startIndexSSBO;
    GLuint _outputDataSSBO;
    GLuint _outputValidSSBO; // For the second output buffer
    GLuint _outputsamplePositionsSSBO; // For the sample positions output buffer
    bool _GPUFullDataModeBuffersInitialized = false;

    mv::Framebuffer _framebuffer;
    GLuint _defaultFramebuffer;

    QMatrix4x4 _modelMatrix;
    QMatrix4x4 _mvpMatrix;

    TrackballCamera _camera;
    mv::Dataset<Volumes> _volumeDataset;
    mv::Dataset<Images> _tfDataset;
    mv::Dataset<Points> _reducedPosDataset;
    mv::Dataset<Images> _materialTransitionDataset;
    mv::Dataset<Images> _materialPositionDataset;

    QSize _screenSize;
    mv::Vector3f _volumeSize = mv::Vector3f{50, 50, 50};
    mv::Vector3f _volumeTextureSize;
    mv::Vector3f _renderSpace = mv::Vector3f{ 50, 50, 50 };
    QPair<float, float> _scalarVolumeDataRange;
    QPair<float, float> _scalarImageDataRange;
    QVector<float> _tfImage;                        // storage for the transfer function data
    QVector<float> _materialPositionImage;          // storage for the material transfer function data
    std::vector<float> _tfSumedAreaTable;           // Is extracted from the final row of the tfDataset 
    std::vector<float> _textureData;                // storage for the volume data, needed for the renderCubes 
    std::vector<int> _neighbourIndicesTexture;      // A marching cubes like texture containing the indices of the neighbours of each voxel 

    float _stepSize = 0.5f;
    mv::Vector3f _cameraPos;

    size_t _fullDataMemorySize = 0; // The size of the full data in bytes
    size_t _fullGPUMemorySize = static_cast<size_t>(14 * 1024 * 1024) * 1024; // The size of the full data in bytes on the GPU if we use normal int it causes a overflow; // The size of the full data in bytes on the GPU

    // HNSWLib-related members  
    std::unique_ptr<hnswlib::L2Space> _hnswSpace;
    std::unique_ptr<hnswlib::HierarchicalNSW<float>> _hnswIndex;
    int _hnswM = 8;
    int _hnswEfConstruction = 50;
    int _hwnsEfSearch = 50;
    
    // Full Data Rendermode Parameters
    std::vector<std::vector<int>> _GPUBatches;
    std::vector<std::vector<int>> _GPUBatchesStartIndex;
    std::vector<size_t> _subsetsMemory; // Total memory per batch.
    int _fullDataModeBatch = -1;

    // Marching cubes tables (for smoothing in NN modes)
    int* edgeTable = MarchingCubes::getEdgeTable();
    int* triTable = MarchingCubes::getTriTable();

    // Calculate the size of the data in bytes
    size_t edgeTableSize = sizeof(MarchingCubes::edgeTable);
    size_t triTableSize = sizeof(MarchingCubes::triTable);
};

