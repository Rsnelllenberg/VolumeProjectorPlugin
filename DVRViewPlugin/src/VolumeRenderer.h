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
#include <faiss/IndexIVFFlat.h>
#include <faiss/IndexFlat.h>
#include <faiss/Index.h>

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

struct BatchTiming {
    int    batchIndex = -1;
    double annTime = 0.0;  // ANN training (first batch only)
    double prepTime = 0.0;  // Data preparation (memory checks, etc.)
    double collectTime = 0.0;  // retrieveBatchFullData()
    double searchTime = 0.0;  // batchSearch()
    double renderTime = 0.0;  // renderBatchToScreen()
    double totalTime = 0.0;  // Whole batch
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
    void setUseClutterRemover(bool ClutterRemoval);
    void setUseShading(bool useShading);

    void setRenderCubeSize(float renderCubeSize);

    void loadNNVolumeToTexture(mv::Texture3D& targetVolume, std::vector<float>& textureData, QVector<float>& usedTFImage, int width, mv::Vector3f volumeSize, int pointAmount, bool singleValueTFTexture);

    void updataDataTexture();

    mv::Vector3f getVolumeSize() { return _volumeSize; }
    bool getFullRenderModeInProgress() { return _fullDataModeBatch != -1; }

    void init();
    void resize(QSize renderSize);

    void setDefaultRenderSettings();

    void render();
    void destroy();

    void cycleHWNSParameters();

    void benchmarkAllRenderModes(const std::string& outputCsvPath, int runs);

private:
    void renderDirections();
    void renderTexture(mv::Texture2D& texture);
    void updateMatrices();

    void drawDVRRender(mv::ShaderProgram& shader);
    void drawDVRQuad(mv::ShaderProgram& shader);

    // Full data render mode methods
    void prepareANN();
    void batchSearch(const std::vector<float>& queryData, std::vector<float>& positionData, uint32_t dimensions, int k, bool useWeightedMean, std::vector<float>& meanPositionData);
    void getFacesTextureData(std::vector<float>& frontfacesData, std::vector<float>& backfacesData);
    void getGPUFullDataModeBatches(std::vector<float>& frontfacesData, std::vector<float>& backfacesData);
    void retrieveBatchFullData(std::vector<float>& cpuOutput, int batchIndex, bool deleteBuffers);
    void renderBatchToScreen(int batchIndex, uint32_t sampleDim, std::vector<float>& meanPositions);
    QVector2D ComputeMeanOfNN(const std::vector<std::pair<float, int64_t>>& neighbors, int k, const std::vector<float>& positionData);
    void updateRenderModeParameters();

    void renderFullData();
    void renderComposite2DPos();
    void renderCompositeColor();
    void render1DMip();

    void renderMaterialTransition2D();
    void renderNNMaterialTransition();
    void renderAltNNMaterialTransition();

    void normalizePositionData(std::vector<float>& positionData);

    void updateRenderCubes();
    void renderFullDataBenchmark(std::vector<BatchTiming>& outTimings);
    size_t getTotalSamples();
    void benchmarkFullDataMode(const std::string& summaryCsv, const std::string& detailedCsv, int runs);


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
    mv::ShaderProgram _fullDataMaterialTransitionShader;
    QOpenGLShaderProgram* _fullDataSamplerComputeShader; // This has a different type since mv::ShaderProgram does not support compute shaders

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
    bool _useClutterRemover = false; // only works for a few render modes, such as the NNMaterialTransition renderMode
    bool _useShading = false;
    bool _ANNAlgorithmTrained = false; 

    // (WIP) Originally this was used for empty space skipping, but it wasn't fully implemented and now this part is left in solly for the purpose having the camera kind of work inside the volume
    int _renderCubeSize = 20;
    int _renderCubeAmount = 1;

    mv::Texture2D _frontfacesTexture;
    mv::Texture2D _backfacesTexture;
    mv::Texture2D _depthTexture;
    mv::Texture2D _prevFullCompositeTexture; // The previous screen texture, used for the full data mode
    mv::Texture2D _adaptedScreenSizeTexture; // The texture with the size of the adaptedScreensize, used as an intermediate texture for all rendermode such that they effectivly render at any resolution and then later upscaled to the screen size

    mv::Texture2D _tfTexture;                   //2D texture containing the transfer function
    mv::Texture2D _materialTransitionTexture;   //2D texture containing the material transition texture
    mv::Texture2D _materialPositionTexture;     //2D texture containing the material position texture
    mv::Texture3D _volumeTexture;               //3D texture containing the volume data

    mv::Texture3D _tempNNMaterialVolume; // Temporary texture used for the NN material transition rendering, it is used to store the material volume data that is used to clean up noisy material transitions

    // IDs for the render cube buffers
    // (WIP) Originally this was used for empty space skipping, but it wasn't fully implemented and now this part is left in solly for the purpose having the camera kind of work inside the volume
    GLuint _renderCubePositionsBufferID;
    GLuint _renderCubePositionsTexID;


    // Create and bind the Marching cubes SSBOs
    GLuint edgeTableSSBO, triTableSSBO;

    //Large GPU buffers for the full data mode
    GLuint _indicesSSBO;
    GLuint _startIndexSSBO;
    GLuint _outputDataSSBO;
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

    QSize _adjustedScreenSize;
    mv::Vector3f _volumeSize = mv::Vector3f{50, 50, 50};
    mv::Vector3f _volumeTextureSize;
    mv::Vector3f _renderSpace = mv::Vector3f{ 50, 50, 50 };
    QPair<float, float> _scalarVolumeDataRange;
    QPair<float, float> _scalarImageDataRange;
    QVector<float> _tfImage;                        // storage for the transfer function data
    QVector<float> _materialPositionImage;          // storage for the material transfer function data
    std::vector<float> _textureData;                // Storage for the volume data, currently used as a temporary storage for the volume data that is loaded into the texture (The fullDataRenderMode will use it for some auxiliary data so it won't reliably actually contain the current value there)
    float _stepSize = 0.5f;
    mv::Vector3f _cameraPos;

    size_t _fullDataMemorySize = 0; // The size of the full data in bytes
    size_t _fullGPUMemorySize = static_cast<size_t>(14 * 1024 * 1024) * 1024; // The size of the full data in bytes on the GPU if we use normal int it causes a overflow; The SSBOs are limited to 2GB, so even if the GPU has more VRAM we limit the size to 2GB for the full data mode.

    // ANN-related members  
    std::unique_ptr<hnswlib::L2Space> _hnswSpace;
    std::unique_ptr<hnswlib::HierarchicalNSW<float>> _hnswIndex;
    std::unique_ptr<faiss::IndexIVFFlat> _faissIndexIVF;
    std::unique_ptr<faiss::IndexFlatL2> _faissIndexFlat;
    int _hnswM = 16;
    int _hnswEfConstruction = 200;
    int _hnswEfSearch = 200;

    int _nlist = 1000;
    int _nprobe = 1; // Number of probes for Faiss IVF index

    // Boolean to select ANN library
    bool _useFaissANN = false;

    // Benchmarking and profiling
    int _testStage = 1; 
    std::vector<BatchTiming> _batchTimings;
    
    // Full Data Rendermode Parameters
    std::vector<std::vector<int>> _GPUBatches; // Batches of pixel indices for the full data mode as it is not always possible to fit all pixels in one batch
    std::vector<std::vector<int>> _GPUBatchesStartIndex; // Start index for each ray as if each sample takes one space (we multiply by 2 in the shader)
    std::vector<size_t> _subsetsMemory; // Total memory per batch.
    int _fullDataModeBatch = -1; // The batch index of the full data mode that is currently being processed

    // Marching cubes tables (for smoothing in NN modes)
    int* edgeTable = MarchingCubes::getEdgeTable();
    int* triTable = MarchingCubes::getTriTable();

    // Calculate the size of the data in bytes
    size_t edgeTableSize = sizeof(MarchingCubes::edgeTable);
    size_t triTableSize = sizeof(MarchingCubes::triTable);

    bool useWeightedMean = true;  // change to "true" if you need weighting.
    bool useLargestCluster = true; // change to "false" if you do not want to use the largest cluster.

    // Union-Find structure for connected components (used full data render pipeline to remove outliers)
    struct UnionFind {
        std::vector<int> parent;
        UnionFind(int n) : parent(n) { std::iota(parent.begin(), parent.end(), 0); }
        int findRoot(int x) {
            return parent[x] == x ? x : (parent[x] = findRoot(parent[x]));
        }
        void unify(int a, int b) {
            int ra = findRoot(a), rb = findRoot(b);
            if (ra != rb) parent[rb] = ra;
        }
    };

};

