#include "VolumeRenderer.h"
#include <QImage>
#include <random>
#include <QOpenGLWidget>
#include <queue>
#include <algorithm>
#include <numeric>
#include <sstream> 

#ifdef _OPENMP
#include <omp.h>
#endif

void VolumeRenderer::init()
{
    qDebug() << "Initializing VolumeRenderer";

    initializeOpenGLFunctions();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    _volumeTexture.create();
    _volumeTexture.initialize();

    // Initialize the transfer function textures
    _tfTexture.create();
    _tfTexture.bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    _materialTransitionTexture.create();
    _materialTransitionTexture.bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // This one should not use linear interpolation as it is a discrete tf
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    _materialPositionTexture.create();
    _materialPositionTexture.bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Initialize buffers needed for the empty space skipping rendercubes (WIP no empty space skipping was implemented but this was left in as a way to have a camera work inside the volume)
    glGenBuffers(1, &_renderCubePositionsBufferID);
    glBindBuffer(GL_TEXTURE_BUFFER, _renderCubePositionsBufferID);
    glBufferData(GL_TEXTURE_BUFFER, 1, NULL, GL_DYNAMIC_DRAW); // Size will be set later
    glGenTextures(1, &_renderCubePositionsTexID);
    glBindTexture(GL_TEXTURE_BUFFER, _renderCubePositionsTexID);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, _renderCubePositionsBufferID);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    // initialize framebuffer textures and the framebuffer itself
    _frontfacesTexture.create();
    _frontfacesTexture.bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    _backfacesTexture.create();
    _backfacesTexture.bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    _prevFullCompositeTexture.create();
    _prevFullCompositeTexture.bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    _adaptedScreenSizeTexture.create();
    _adaptedScreenSizeTexture.bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    _depthTexture.create();
    _depthTexture.bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    _framebuffer.create();
    _framebuffer.bind();
    _framebuffer.validate();

    // Initialize the volume shader program
    bool loaded = true;
    loaded &= _surfaceShader.loadShaderFromFile(":shaders/Surface.vert", ":shaders/Surface.frag");
    loaded &= _2DCompositeShader.loadShaderFromFile(":shaders/Quad.vert", ":shaders/2DComposite.frag");
    loaded &= _colorCompositeShader.loadShaderFromFile(":shaders/Quad.vert", ":shaders/ColorComposite.frag");
    loaded &= _1DMipShader.loadShaderFromFile(":shaders/Quad.vert", ":shaders/1DMip.frag");
    loaded &= _materialTransition2DShader.loadShaderFromFile(":shaders/Quad.vert", ":shaders/MaterialTransition2D.frag");
    loaded &= _nnMaterialTransitionShader.loadShaderFromFile(":shaders/Quad.vert", ":shaders/NNMaterialTransition.frag");
    loaded &= _altNNMaterialTransitionShader.loadShaderFromFile(":shaders/Quad.vert", ":shaders/AltNNMaterialTransition.frag");
    loaded &= _fullDataCompositeShader.loadShaderFromFile(":shaders/Quad.vert", ":shaders/FullDataCompositeBlending.frag");
    loaded &= _fullDataMaterialTransitionShader.loadShaderFromFile(":shaders/Quad.vert", ":shaders/FullDataMaterialBlending.frag");
    loaded &= _textureShader.loadShaderFromFile(":shaders/Quad.vert", ":shaders/Texture.frag");

    if (!loaded) {
        qCritical() << "Failed to load one of the Volume Renderer shaders";
    }
    else {
        qDebug() << "Volume Renderer shaders loaded";
    }

    // Create the shader program instance.
    _fullDataSamplerComputeShader = new QOpenGLShaderProgram();
    if (!_fullDataSamplerComputeShader->addShaderFromSourceFile(QOpenGLShader::Compute, ":shaders/FullDataSampling.comp"))
    {
        qCritical() << "Failed to load compute shader:" << _fullDataSamplerComputeShader->log();
        return;
    }

    // Initialize the Marching Cubes edge and triangle tables for the smoothing in the NN rendering modes 
    // Create and bind the edgeTable buffer
    glGenBuffers(1, &edgeTableSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, edgeTableSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, edgeTableSize, edgeTable, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, edgeTableSSBO); // Binding 0, 1, 2 are used in the full data sampler shaders, 4 is linked to the current shader

    // Create and bind the triTable buffer
    glGenBuffers(1, &triTableSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, triTableSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, triTableSize, triTable, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, triTableSSBO); // Binding 0, 1, 2 are used in the full data sampler shaders, 5 is linked to the current shader

    // Unbind the buffer
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Initialize a cube mesh 
    const std::array<float, 24> verticesCube{
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f,
    };

    const std::array<unsigned, 36> indicesCube{
        0, 6, 4,
        0, 2, 6,
        0, 3, 2,
        0, 1, 3,
        2, 7, 6,
        2, 3, 7,
        4, 6, 7,
        4, 7, 5,
        0, 4, 5,
        0, 5, 1,
        1, 5, 7,
        1, 7, 3
    };

    const std::array<float, 12> verticesQuad{
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f
    };

    const std::array<unsigned, 6> indicesQuad{
        0, 2, 1,
        1, 2, 3
    };

    // Cube arrays
    _vao.create();
    _vao.bind();

    // Quad arrays
    _vboQuad.create();
    _vboQuad.bind();
    _vboQuad.allocate(verticesQuad.data(), verticesQuad.size() * sizeof(float));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    _iboQuad.create();
    _iboQuad.bind();
    _iboQuad.allocate(indicesQuad.data(), indicesQuad.size() * sizeof(unsigned));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    // Cube arrays
    _vboCube.create();
    _vboCube.bind();
    _vboCube.allocate(verticesCube.data(), verticesCube.size() * sizeof(float));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    _iboCube.create();
    _iboCube.bind();
    _iboCube.allocate(indicesCube.data(), indicesCube.size() * sizeof(unsigned));

    _vao.release();
    _vboCube.release();
    _iboCube.release();
    qDebug() << "VolumeRenderer initialized";


}


void VolumeRenderer::resize(QSize renderSize)
{

    _adjustedScreenSize = renderSize * 1.0f;


    _backfacesTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, _adjustedScreenSize.width(), _adjustedScreenSize.height(), 0, GL_RGB, GL_FLOAT, nullptr);

    _frontfacesTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, _adjustedScreenSize.width(), _adjustedScreenSize.height(), 0, GL_RGB, GL_FLOAT, nullptr);

    _prevFullCompositeTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, _adjustedScreenSize.width(), _adjustedScreenSize.height(), 0, GL_RGB, GL_FLOAT, nullptr);

    _adaptedScreenSizeTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, _adjustedScreenSize.width(), _adjustedScreenSize.height(), 0, GL_RGB, GL_FLOAT, nullptr);

    _depthTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, _adjustedScreenSize.width(), _adjustedScreenSize.height(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glViewport(0, 0, renderSize.width(), renderSize.height());

}

void VolumeRenderer::setData(const mv::Dataset<Volumes>& dataset)
{
    _volumeDataset = dataset;
    _volumeSize = dataset->getVolumeSize().toVector3f();
    _ANNAlgorithmTrained = false; // We need to retrain the ANN algorithm as the data has changed
    _fullDataMemorySize = _volumeSize.x * _volumeSize.y * _volumeSize.z * _volumeDataset->getComponentsPerVoxel() * sizeof(float); // in bytes
    if (_fullGPUMemorySize - _fullDataMemorySize < 0)
    {
        qCritical() << "VolumeRenderer::setData: Not enough GPU memory available for the volume data with set VRAM do not use full data renderModes or change VRAM parameter if you have more available";
        return;
    }

    updataDataTexture();
    updateRenderCubes();
}

void VolumeRenderer::setTfTexture(const mv::Dataset<Images>& tfTexture)
{
    _tfDataset = tfTexture;
    QSize textureDims = _tfDataset->getImageSize();
    int dataSize = textureDims.width() * textureDims.height() * 4;
    _tfImage = QVector<float>(dataSize);
    QPair<float, float> scalarDataRange;
    _tfDataset->getImageScalarData(0, _tfImage, scalarDataRange);

    _scalarImageDataRange = scalarDataRange;

    _tfTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, textureDims.width(), textureDims.height() - 1, 0, GL_RGBA, GL_FLOAT, _tfImage.data());
    _tfTexture.release();

    // In these rendermodes the new dataset will impact the visualization and thus needs to be updated now 
    if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_COLOR || _renderMode == RenderMode::NN_MULTIDIMENSIONAL_COMPOSITE || _renderMode == RenderMode::NN_MaterialTransition || _renderMode == RenderMode::Alt_NN_MaterialTransition || _renderMode == RenderMode::Smooth_NN_MaterialTransition)
        updataDataTexture();
}

void VolumeRenderer::setReducedPosData(const mv::Dataset<Points>& reducedPosData)
{
    _reducedPosDataset = reducedPosData;
    if (!_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_FULL && !_renderMode == RenderMode::MaterialTransition_FULL && _renderMode != RenderMode::MIP) {
        updataDataTexture(); // The position data is used in the rendering process, so we need to update the data texture (apart from the MIP and full data render modes that either don't need it or define it elsewhere)
    }
}

void VolumeRenderer::setMaterialTransitionTexture(const mv::Dataset<Images>& materialTransitionData)
{
    _materialTransitionDataset = materialTransitionData;
    QSize textureDims = _materialTransitionDataset->getImageSize();
    QVector<float> transitionData = QVector<float>(textureDims.width() * textureDims.height() * 4);
    QPair<float, float> scalarDataRange;
    _materialTransitionDataset->getImageScalarData(0, transitionData, scalarDataRange);

    _materialTransitionTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, textureDims.width(), textureDims.height(), 0, GL_RGBA, GL_FLOAT, transitionData.data());
    _materialTransitionTexture.release();
}

void VolumeRenderer::setMaterialPositionTexture(const mv::Dataset<Images>& materialPositionTexture)
{
    _materialPositionDataset = materialPositionTexture;
    QSize textureDims = _materialPositionDataset->getImageSize();
    _materialPositionImage = QVector<float>(textureDims.width() * textureDims.height());
    QPair<float, float> scalarDataRange;
    _materialPositionDataset->getImageScalarData(0, _materialPositionImage, scalarDataRange);

    _materialPositionTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, textureDims.width(), textureDims.height(), 0, GL_RED, GL_FLOAT, _materialPositionImage.data());
    _materialPositionTexture.release();
}

void VolumeRenderer::normalizePositionData(std::vector<float>& positionData)
{
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::min();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::min();

    for (int i = 0; i < positionData.size(); i++) {
        if (i % 2 == 0) {
            if (positionData[i] < minX)
                minX = positionData[i];
            if (positionData[i] > maxX)
                maxX = positionData[i];
        }
        else {
            if (positionData[i] < minY)
                minY = positionData[i];
            if (positionData[i] > maxY)
                maxY = positionData[i];
        }
    }

    float rangeX = maxX - minX;
    float rangeY = maxY - minY;

    int size = _tfDataset->getImageSize().width(); // We use a square texture so width is also height
    if (_renderMode == RenderMode::MaterialTransition_2D || _renderMode == RenderMode::NN_MaterialTransition || _renderMode == RenderMode::Alt_NN_MaterialTransition || _renderMode == RenderMode::MaterialTransition_FULL)
        size = _materialPositionDataset->getImageSize().width();

    for (int i = 0; i < positionData.size(); i += 2)
    {
        positionData[i] = ((positionData[i] - minX) / rangeX) * (size - 1);
        positionData[i + 1] = ((positionData[i + 1] - minY) / rangeY) * (size - 1);
    }
}

void VolumeRenderer::updateRenderCubes()
{
    mv::Vector3f relativeBlockSize = mv::Vector3f(_renderCubeSize / _volumeSize.x, _renderCubeSize / _volumeSize.y, _renderCubeSize / _volumeSize.z);

    int nx = std::ceil(1.0f / relativeBlockSize.x);
    int ny = std::ceil(1.0f / relativeBlockSize.y);
    int nz = std::ceil(1.0f / relativeBlockSize.z);

    std::vector<mv::Vector3f> positions;
    std::vector<float> occupancyValues;

    for (int x = 0; x < nx; ++x)
    {
        for (int y = 0; y < ny; ++y)
        {
            for (int z = 0; z < nz; ++z)
            {
                mv::Vector3f posIndex = mv::Vector3f(x, y, z);
                positions.push_back(posIndex);
            }
        }
    }
    qDebug() << "Amount of render cubes: " << positions.size();
    glBindBuffer(GL_TEXTURE_BUFFER, _renderCubePositionsBufferID);
    glBufferData(GL_TEXTURE_BUFFER, positions.size() * sizeof(mv::Vector3f), positions.data(), GL_DYNAMIC_DRAW);

    _renderCubeAmount = positions.size();
}

// This function handles the loading of volume data that requires the results of the transfer function to already be aplied to the data before being stored in the texture.
void VolumeRenderer::loadNNVolumeToTexture(mv::Texture3D& targetVolume, std::vector<float>& textureData, QVector<float>& usedTFImage, int width, mv::Vector3f volumeSize, int pointAmount, bool singleValueTFTexture)
{
    if (!_reducedPosDataset.isValid()) {
        qCritical() << "No DR reduction data set";
        return;
    }
    textureData = std::vector<float>(pointAmount * 4);

    //Get the correct data into textureData 
    std::vector<float> positionData = std::vector<float>(pointAmount * 2);
    _reducedPosDataset->populateDataForDimensions(positionData, std::vector<int>{0, 1});
    normalizePositionData(positionData);

    for (int i = 0; i < pointAmount; i++)
    {
        int x = positionData[i * 2];
        int y = positionData[i * 2 + 1];
        if (singleValueTFTexture) { // If we only have a single field, we need to use the same value for all channels
            int pixelPos = (y * width + x);

            textureData[i * 4] = usedTFImage[pixelPos];
            textureData[(i * 4) + 1] = usedTFImage[pixelPos];
            textureData[(i * 4) + 2] = usedTFImage[pixelPos];
            textureData[(i * 4) + 3] = usedTFImage[pixelPos];
        }
        else { // If we have multiple fields, we need to use the correct value for each channel
            int pixelPos = (y * width + x) * 4;
            textureData[i * 4] = usedTFImage[pixelPos];
            textureData[(i * 4) + 1] = usedTFImage[pixelPos + 1];
            textureData[(i * 4) + 2] = usedTFImage[pixelPos + 2];
            textureData[(i * 4) + 3] = usedTFImage[pixelPos + 3];
        }
    }
    // Generate and bind a 3D texture
    targetVolume.bind();
    targetVolume.setData(volumeSize.x, volumeSize.y, volumeSize.z, textureData, 4);
    targetVolume.release(); // Unbind the texture
}

void VolumeRenderer::updataDataTexture()
{
    QPair<float, float> scalarDataRange;


    if (_volumeDataset.isValid()) {
        if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_FULL || _renderMode == RenderMode::MaterialTransition_FULL) {
            int blockAmount = std::ceil(float(_compositeIndices.size()) / 4.0f) * 4; //Since we always assume textures with 4 dimensions all of which need to be filled
            _textureData = std::vector<float>(blockAmount * _volumeDataset->getNumberOfVoxels());
            _volumeTextureSize = _volumeDataset->getVolumeAtlasData(_compositeIndices, _textureData, scalarDataRange);
            _fullDataMemorySize = sizeof(float) * _textureData.size(); // in bytes
            qDebug() << "Full data memory size: " << _fullDataMemorySize;
            // Generate and bind a 3D texture
            _volumeTexture.bind();
            _volumeTexture.setData(_volumeTextureSize.x, _volumeTextureSize.y, _volumeTextureSize.z, _textureData, 4);
            _volumeTexture.release(); // Unbind the texture
        }
        else if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_2D_POS || _renderMode == RenderMode::MaterialTransition_2D) {
            if (!_tfDataset.isValid() || !_reducedPosDataset.isValid()) { // _tfTexture is used in the normalize function
                qCritical() << "No position data set";
                return;
            }
            _textureData = std::vector<float>(_volumeDataset->getNumberOfVoxels() * 2);
            _volumeTextureSize = _volumeSize;

            _reducedPosDataset->populateDataForDimensions(_textureData, std::vector<int>{0, 1});
            normalizePositionData(_textureData);

            // Generate and bind a 3D texture
            _volumeTexture.bind();
            _volumeTexture.setData(_volumeTextureSize.x, _volumeTextureSize.y, _volumeTextureSize.z, _textureData, 2);
            _volumeTexture.release(); // Unbind the texture
        }
        else if (_renderMode == RenderMode::NN_MaterialTransition || _renderMode == RenderMode::Alt_NN_MaterialTransition || _renderMode == RenderMode::Smooth_NN_MaterialTransition) {
            if (!_materialPositionDataset.isValid()) {
                qCritical() << "No transfer function data set";
                return;
            }

            _volumeTextureSize = _volumeSize;
            loadNNVolumeToTexture(_volumeTexture, _textureData, _materialPositionImage, _materialPositionDataset->getImageSize().width(), _volumeTextureSize, _volumeDataset->getNumberOfVoxels(), true);
        }
        else if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_COLOR || _renderMode == RenderMode::NN_MULTIDIMENSIONAL_COMPOSITE) {
            if (!_tfDataset.isValid()) {
                qCritical() << "No transfer function data set";
                return;
            }
            
            _volumeTextureSize = _volumeSize;
            loadNNVolumeToTexture(_volumeTexture, _textureData, _tfImage, _tfDataset->getImageSize().width(), _volumeTextureSize, _volumeDataset->getNumberOfVoxels(), false);
        }
        else if (_renderMode == RenderMode::MIP) {
            _textureData = std::vector<float>(_volumeDataset->getNumberOfVoxels());
            _volumeTextureSize = _volumeDataset->getVolumeAtlasData(std::vector<uint32_t>{ uint32_t(_mipDimension) }, _textureData, scalarDataRange, 1);

            // Generate and bind a 3D texture
            _volumeTexture.bind();
            _volumeTexture.setData(_volumeTextureSize.x, _volumeTextureSize.y, _volumeTextureSize.z, _textureData, 1);
            _volumeTexture.release(); // Unbind the texture
        }
        else
            qCritical() << "Unknown render mode";
    }
    else
        qCritical() << "No volume data set";

    _scalarVolumeDataRange = scalarDataRange;
}

void VolumeRenderer::setCamera(const TrackballCamera& camera)
{
    _camera = camera;
}

void VolumeRenderer::setDefaultFramebuffer(GLuint defaultFramebuffer)
{
    _defaultFramebuffer = defaultFramebuffer;
}

void VolumeRenderer::setClippingPlaneBoundery(mv::Vector3f min, mv::Vector3f max)
{
    _minClippingPlane = min;
    _maxClippingPlane = max;
}

void VolumeRenderer::setStepSize(float stepSize)
{
    _stepSize = stepSize;
}

void VolumeRenderer::setRenderSpace(mv::Vector3f size)
{
    _renderSpace = size;
}

void VolumeRenderer::setUseCustomRenderSpace(bool useCustomRenderSpace)
{
    _useCustomRenderSpace = useCustomRenderSpace;
}

// Which dimension should we send to the GPU (used for the full data and MIP render modes)
void VolumeRenderer::setCompositeIndices(std::vector<std::uint32_t> compositeIndices)
{
    if (_compositeIndices != compositeIndices)
        _dataSettingsChanged = true;
    _compositeIndices = compositeIndices;
}

// Converts render mode string to enum and saves it.
// Possible strings are: 
// "MaterialTransition Full", 
// "MaterialTransition 2D", 
// "NN MaterialTransition", 
// "Alt NN MaterialTransition", 
// "Smooth NN MaterialTransition", 
// "MultiDimensional Composite Full", 
// "MultiDimensional Composite 2D Pos", 
// "MultiDimensional Composite Color", 
// "NN MultiDimensional Composite", 
// "1D MIP"
void VolumeRenderer::setRenderMode(const QString& renderMode)
{
    RenderMode givenMode;
    if (renderMode == "MaterialTransition Full")
        givenMode = RenderMode::MaterialTransition_FULL;
    else if (renderMode == "MaterialTransition 2D")
        givenMode = RenderMode::MaterialTransition_2D;
    else if (renderMode == "NN MaterialTransition")
        givenMode = RenderMode::NN_MaterialTransition;
    else if (renderMode == "Smooth NN MaterialTransition")
        givenMode = RenderMode::Smooth_NN_MaterialTransition;
    else if (renderMode == "Alt NN MaterialTransition")
        givenMode = RenderMode::Alt_NN_MaterialTransition;
    else if (renderMode == "MultiDimensional Composite Full")
        givenMode = RenderMode::MULTIDIMENSIONAL_COMPOSITE_FULL;
    else if (renderMode == "MultiDimensional Composite 2D Pos")
        givenMode = RenderMode::MULTIDIMENSIONAL_COMPOSITE_2D_POS;
    else if (renderMode == "MultiDimensional Composite Color")
        givenMode = RenderMode::MULTIDIMENSIONAL_COMPOSITE_COLOR;
    else if (renderMode == "NN MultiDimensional Composite")
        givenMode = RenderMode::NN_MULTIDIMENSIONAL_COMPOSITE;
    else if (renderMode == "1D MIP")
        givenMode = RenderMode::MIP;
    else
        qCritical() << "Unknown render mode";

    // Group render modes by needed volume texture requirements
    auto getRenderModeGroup = [](RenderMode mode) {
        if (mode == RenderMode::MaterialTransition_FULL || mode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_FULL)
            return 1;
        if (mode == RenderMode::MaterialTransition_2D)
            return 2;
        if (mode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_2D_POS)
            return 3;
        if (mode == RenderMode::NN_MaterialTransition || mode == RenderMode::Alt_NN_MaterialTransition || mode == RenderMode::Smooth_NN_MaterialTransition)
            return 4;
        if (mode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_COLOR || mode == RenderMode::NN_MULTIDIMENSIONAL_COMPOSITE)
            return 5;
        if (mode == RenderMode::MIP)
            return 6;
        return 0; // Unknown group
        };

    int currentGroup = getRenderModeGroup(givenMode);

    // Only set _dataSettingsChanged to true if the group changes
    if (getRenderModeGroup(_renderMode) != currentGroup)
        _dataSettingsChanged = true;

    if (currentGroup == 1 || currentGroup == 2 || currentGroup == 3 || currentGroup == 6) {
        // For these groups we need to ensure that the volume texture is bound and has the correct settings
        _volumeTexture.bind();
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        _volumeTexture.release(); // Unbind the texture
    }

    if (currentGroup == 4) {
        _volumeTexture.bind();
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        _volumeTexture.release(); // Unbind the texture
    }

    // The NN and Linear version of the same render mode share the same volume texture but they do require slightly different settings
    if (currentGroup == 5) {
        _volumeTexture.bind();
        if (givenMode == RenderMode::NN_MULTIDIMENSIONAL_COMPOSITE) {
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        }
        else {
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        _volumeTexture.release(); // Unbind the texture
    }

    if (currentGroup != 1) {
        _fullDataModeBatch = -1; // We don't need to use the full data in these modes, so we reset the batch progress counter
    }

    _renderMode = givenMode;
}

void VolumeRenderer::setMIPDimension(int mipDimension)
{
    if (_mipDimension != mipDimension)
        _dataSettingsChanged = true;
    _mipDimension = mipDimension;
}

void VolumeRenderer::setUseClutterRemover(bool ClutterRemoval)
{
    _useClutterRemover = ClutterRemoval;
}

void VolumeRenderer::setUseShading(bool useShading)
{
    _useShading = useShading;
}

void VolumeRenderer::setRenderCubeSize(float renderCubeSize)
{
    if (_renderCubeSize != renderCubeSize) {
        _renderCubeSize = renderCubeSize;
        updateRenderCubes();
    }
}

void VolumeRenderer::updateMatrices()
{
    QVector3D cameraPos = _camera.getPosition();
    _cameraPos = mv::Vector3f(cameraPos.x(), cameraPos.y(), cameraPos.z());

    // Create the model-view-projection matrix
    QMatrix4x4 modelMatrix;
    if (_useCustomRenderSpace)
        modelMatrix.scale(_renderSpace.x, _renderSpace.y, _renderSpace.z);
    else
        modelMatrix.scale(_volumeSize.x, _volumeSize.y, _volumeSize.z);
    _modelMatrix = modelMatrix;
    _mvpMatrix = _camera.getProjectionMatrix() * _camera.getViewMatrix() * _modelMatrix;
}

void VolumeRenderer::drawDVRRender(mv::ShaderProgram& shader)
{
    _vboCube.bind();
    _iboCube.bind();

    //pass the texture buffers to the shader
    glActiveTexture(GL_TEXTURE5); //We start from texture 5 to avoid overlapping textures 
    glBindTexture(GL_TEXTURE_BUFFER, _renderCubePositionsTexID);
    shader.uniform1i("renderCubePositions", 5);

    shader.uniformMatrix4f("u_modelViewProjection", _mvpMatrix.constData());
    shader.uniformMatrix4f("u_model", _modelMatrix.constData());
    shader.uniform3fv("u_minClippingPlane", 1, &_minClippingPlane);
    shader.uniform3fv("u_maxClippingPlane", 1, &_maxClippingPlane);

    mv::Vector3f renderCubeSize = mv::Vector3f(_renderCubeSize / _volumeSize.x, _renderCubeSize / _volumeSize.y, _renderCubeSize / _volumeSize.z);
    shader.uniform3fv("renderCubeSize", 1, &renderCubeSize);

    // The actual rendering step
    _vao.bind();
    glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, _renderCubeAmount);
}

void VolumeRenderer::drawDVRQuad(mv::ShaderProgram& shader)
{
    shader.uniform3fv("u_minClippingPlane", 1, &_minClippingPlane);
    shader.uniform3fv("u_maxClippingPlane", 1, &_maxClippingPlane);

    _vboQuad.bind();
    _iboQuad.bind();
    _vao.bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

}

// Shared function for all rendertypes, it calculates the ray direction and lengths for each pixel
void VolumeRenderer::renderDirections()
{
    _framebuffer.bind();
    _framebuffer.setTexture(GL_DEPTH_ATTACHMENT, _depthTexture);
    _framebuffer.setTexture(GL_COLOR_ATTACHMENT0, _frontfacesTexture);

    // Set correct settings
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);

    // Render frontfaces into a texture
    _surfaceShader.bind();
    drawDVRRender(_surfaceShader);

    // Render backfaces into a texture
    _framebuffer.setTexture(GL_COLOR_ATTACHMENT0, _backfacesTexture);

    // We count missed rays as very close to the camera since we want to select the furthest away geometry in this renderpass
    glClearDepth(0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_GREATER);

    drawDVRRender(_surfaceShader);

    // Restore depth clear value
    glClearDepth(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);

    _framebuffer.release();
}


void VolumeRenderer::prepareANN()
{
    if (!_volumeDataset.isValid()) {
        qCritical() << "Volume dataset is not valid. Cannot prepare ANN.";
        return;
    }

    uint32_t numVoxels = _volumeDataset->getNumberOfVoxels();
    uint32_t dimensions = _volumeDataset->getComponentsPerVoxel();

    // Populate ANN index with volume data.
    std::vector<float> voxelData(dimensions * numVoxels);
    QPair<float, float> scalarDataRange;
    _volumeDataset->getVolumeData(_compositeIndices, voxelData, scalarDataRange);
#ifdef USE_FAISS
    if (_useFaissANN) {
        _nlist = std::clamp(static_cast<int>(numVoxels / 1000), 32, 4096); // nlist is the number of clusters in Faiss
        //_nprobe = std::clamp(static_cast<int>(numVoxels / 1000000), 1, 64); // nprobe is the number of clusters to search in Faiss

        // IVF index for large datasets
        _faissIndexFlat = std::make_unique<faiss::IndexFlatL2>(dimensions);
        _faissIndexIVF = std::make_unique<faiss::IndexIVFFlat>(_faissIndexFlat.get(), dimensions, _nlist, faiss::METRIC_L2);
        _faissIndexIVF->train(numVoxels, voxelData.data());
        _faissIndexIVF->add(numVoxels, voxelData.data());

        //_faissIndexIVF->nprobe = _nprobe; // Set the number of clusters to search

    }
    else
#endif  
    {
            // Build a filename referencing key parameters
            std::ostringstream oss;
            oss << _hnswIndexFolder << "hnsw_index"
                << "_M" << _hnswM
                << "_efC" << _hnswEfConstruction
                << "_dim" << dimensions
                << "_voxNum" << numVoxels
                << ".bin";
            std::string indexPath = oss.str();

            // Initialize HNSW space
            _hnswSpace = std::make_unique<hnswlib::L2Space>(dimensions);

            if (std::filesystem::exists(indexPath)) {
                // Load existing index
                _hnswIndex = std::make_unique<hnswlib::HierarchicalNSW<float>>(_hnswSpace.get(), indexPath);
                _hnswIndex->setEf(_hwnsEfSearch);
                qDebug() << "Loaded HNSW index from:" << QString::fromStdString(indexPath);
            }
            else {
                // Train and save new index
                _hnswIndex = std::make_unique<hnswlib::HierarchicalNSW<float>>(
                    _hnswSpace.get(),
                    numVoxels,
                    _hnswM,
                    _hnswEfConstruction
                );
                for (uint32_t i = 0; i < numVoxels; ++i) {
                    _hnswIndex->addPoint(voxelData.data() + i * dimensions, i);
                }
                _hnswIndex->setEf(_hwnsEfSearch);

                try {
                    _hnswIndex->saveIndex(indexPath);
                    qDebug() << "HNSW index saved to:" << QString::fromStdString(indexPath);
                }
                catch (const std::exception& e) {
                    qCritical() << "Failed to save HNSW index:" << e.what();
                }
            }
        }
}



// Method that handles large query batches using hsnswlib faster then calling searchKnn for each query in a for loop by making use of parallelization 
// It also handles the weighted mean calculation for the query results
// And it outputs the results into a vector of floats
void VolumeRenderer::batchSearch(
    const std::vector<float>& queryData,    // Flat vector: each query is (dimensions) floats
    std::vector<float>& positionData,       // The 2D position data for the queries
    uint32_t dimensions,                    // Dimensionality of a single query
    int k,                                  // Number of nearest neighbors to retrieve
    bool useWeightedMean,                   // Use weighted mean for the query
    std::vector<float>& meanPositionData   // Output: The mean position data for the queries
) {
    if (queryData.size() % dimensions != 0) {
        qCritical() << "Query data size is not a multiple of dimensions.";
    }

    int64_t numQueries = static_cast<int64_t>(queryData.size() / dimensions);
#ifdef USE_FAISS
    if (_useFaissANN) {
        if (!_faissIndexIVF->is_trained) {
            qCritical() << "Faiss IVF index is not trained!";
            return;
        }

        std::vector<faiss::idx_t> labels(numQueries * k);
        std::vector<float> distances(numQueries * k);

        qDebug() << "Searching for" << k << "nearest neighbors for" << numQueries << "queries using Faiss IVF index.";
        _faissIndexIVF->search(numQueries, queryData.data(), k, distances.data(), labels.data());
        qDebug() << "Faiss IVF search completed.";

        for (int64_t i = 0; i < numQueries; i++) {
            std::vector<std::pair<float, int64_t>> answers;
            for (int j = 0; j < k; j++) {
                answers.emplace_back(distances[i * k + j], labels[i * k + j]);
            }
            // Compute the mean position of the nearest neighbors.
            QVector2D meanPos = ComputeMeanOfNN(answers, k, positionData);
            // Store the mean position in the output vector.
            meanPositionData[i * 2] = meanPos.x();
            meanPositionData[i * 2 + 1] = meanPos.y();
        }
    }
    else 
#endif // USE_FAISS
    {
        // Check that the index is valid.
        if (!_hnswIndex) {
            qCritical() << "HNSW index is not initialized.";
        }

        #pragma omp parallel for schedule(guided)
        for (int64_t i = 0; i < numQueries; i++) { // it is important to use int64_t here to avoid overflow crashes
            // Find pointer to the start of the i-th query.
            const float* query = queryData.data() + static_cast<int64_t>(i * dimensions);
            std::priority_queue<std::pair<float, hnswlib::labeltype>> resultQueue = _hnswIndex->searchKnn(query, k);

            // Convert the priority queue to a vector.
            std::vector<std::pair<float, int64_t>> answers;
            while (!resultQueue.empty()) {
                answers.push_back({ resultQueue.top().first, static_cast<int64_t>(resultQueue.top().second) });
                resultQueue.pop();
            }
            QVector2D meanPos = ComputeMeanOfNN(answers, k, positionData);
            meanPositionData[i * 2] = meanPos.x();
            meanPositionData[i * 2 + 1] = meanPos.y();

        }
    }
}

// Extracts the frontfaces and backfaces texture data into a vector of floats
void VolumeRenderer::getFacesTextureData(std::vector<float>& frontfacesData, std::vector<float>& backfacesData)
{
    _frontfacesTexture.bind();
    // Read the frontfaces texture data (we request RGB – assuming alpha is not needed)
    glBindTexture(GL_TEXTURE_2D, _frontfacesTexture.getHandle());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, frontfacesData.data());

    _backfacesTexture.bind();
    // Read the backfaces texture data
    glBindTexture(GL_TEXTURE_2D, _backfacesTexture.getHandle());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, backfacesData.data());

    // Unbind the textures
    glBindTexture(GL_TEXTURE_2D, 0);
}

// This function is used to get the GPU data in full data mode.
// It uses the frontfaces and backfaces texture data to compute the ray lengths and sample counts for each pixel.
// It then uses this data to create batches of pixels that can be processed in the GPU.
void VolumeRenderer::getGPUFullDataModeBatches(std::vector<float>& frontfacesData, std::vector<float>& backfacesData)
{

    _GPUBatches.clear();
    _GPUBatchesStartIndex.clear();
    _subsetsMemory.clear();

    // Get the dimensions of the textures
    int width = _adjustedScreenSize.width();
    int height = _adjustedScreenSize.height();

    // Check if the frontfaces and backfaces data are valid
    if (frontfacesData.size() != backfacesData.size() || frontfacesData.size() != width * height * 3)
    {
        qCritical() << "Frontfaces and backfaces data size mismatch or invalid size.";
        return;
    }

    //Create small batches of pixels that are spread out over the whole image ---

    int numBatches = 2048; // Number of batches to divide the data into. (Tune as needed.)
    std::vector<std::vector<int>> batches(numBatches); // Each element is a vector of pixel indices.
    // Instead of memory requirements (in bytes), we now record the number of samples per ray.
    std::vector<std::vector<int>> batchRaySampleAmount(numBatches);
    // The total reserved memory for each batch, in bytes, computed from the number of samples.
    std::vector<size_t> batchRayMemoryRequirments(numBatches, 0);

    // Get the per-sample size in bytes; this is used to determine
    // how much space each sample occupies in the output array.
    int dimensions = _volumeDataset->getComponentsPerVoxel();
    size_t sampleSizeBytes = dimensions * sizeof(float);

    mv::Vector3f volumeSize;
    if (_useCustomRenderSpace)
        volumeSize = _renderSpace;
    else
        volumeSize = _volumeSize;

    size_t maxBatchMemory = 0;
    // Process pixels in parallel, grouping them by batch index.
    #pragma omp parallel for
    for (int batchIndex = 0; batchIndex < numBatches; ++batchIndex)
    {
        for (int idx = batchIndex; idx < width * height; idx += numBatches)
        {

            // Get positions from the textures.
            mv::Vector3f frontPos(
                frontfacesData[idx * 3 + 0],
                frontfacesData[idx * 3 + 1],
                frontfacesData[idx * 3 + 2]);
            mv::Vector3f backPos(
                backfacesData[idx * 3 + 0],
                backfacesData[idx * 3 + 1],
                backfacesData[idx * 3 + 2]);

            // Convert to volume space.
            mv::Vector3f absFront = frontPos * volumeSize;
            mv::Vector3f absBack = backPos * volumeSize;

            // Compute ray length.
            mv::Vector3f diff = absBack - absFront;
            if (diff == mv::Vector3f(0.0f, 0.0f, 0.0f))
                continue; // Skip if the ray length is zero (no valid ray).

            float rayLength = std::sqrt(diff.x * diff.x +
                diff.y * diff.y +
                diff.z * diff.z);

            // Record the pixel index.
            batches[batchIndex].push_back(idx);

            // Compute the number of samples along this ray.
            int sampleCount = std::ceil(rayLength / _stepSize);

            batchRaySampleAmount[batchIndex].push_back(sampleCount);
            // Update the batch total (in bytes) for partitioning.
            batchRayMemoryRequirments[batchIndex] += sampleCount * sampleSizeBytes;
            #pragma omp critical
            {
                // Update the maximum memory requirement for this batch.
                if (batchRayMemoryRequirments[batchIndex] > maxBatchMemory)
                    maxBatchMemory = batchRayMemoryRequirments[batchIndex];
            }
        }
    }

    // Combine as many of the small batches as can possibly fit in the indicated GPU memory ---

    // Calculate available GPU memory for the batch transfer
    size_t availableMemoryInBytes = std::min(size_t(_fullGPUMemorySize - _fullDataMemorySize - 100000), (size_t(2 * 1024 * 1024) * 1024)); // ~100MB reserved for other data
    if (availableMemoryInBytes < 0 || availableMemoryInBytes < maxBatchMemory)
        throw std::runtime_error("Not enough GPU memory available for the GPU-CPU batch transfer.");


    std::vector<std::vector<int>> selectedBatchIndicesSubsets;
    std::vector<int> currentSubset;
    size_t currentSubsetMemory = 0; // in bytes
    for (int i = 0; i < numBatches; i++)
    {
        // If adding the current batch would exceed our limit (and currentSubset isn't empty), start a new subset.
        if (!currentSubset.empty() &&
            currentSubsetMemory + batchRayMemoryRequirments[i] > availableMemoryInBytes)
        {
            selectedBatchIndicesSubsets.push_back(currentSubset);
            _subsetsMemory.push_back(currentSubsetMemory);
            qDebug() << "Subset" << selectedBatchIndicesSubsets.size() - 1 << "requires" << currentSubsetMemory / (1024 * 1024) << "MB of GPU memory.";
            currentSubset.clear();
            currentSubsetMemory = 0;
        }

        if (batchRayMemoryRequirments[i] == 0)
            continue; // Skip empty batches

        // Add the current batch index to the current subset.
        currentSubset.push_back(i);
        currentSubsetMemory += batchRayMemoryRequirments[i];
    }
    // Add the last subset (the one that did not cross the memory boundary yet)
    if (!currentSubset.empty())
    {
        selectedBatchIndicesSubsets.push_back(currentSubset);
        _subsetsMemory.push_back(currentSubsetMemory);
        qDebug() << "Subset" << selectedBatchIndicesSubsets.size() - 1 << "requires" << currentSubsetMemory / (1024 * 1024) << "MB of GPU memory.";
    }

    // We now build the GPU batch arrays. For each subset, we combine the batch indices
    // and compute, for each ray, the start offset based on the cumulative sum of its sample counts.
    _GPUBatches.resize(selectedBatchIndicesSubsets.size());
    _GPUBatchesStartIndex.resize(selectedBatchIndicesSubsets.size());
    for (size_t subsetIndex = 0; subsetIndex < selectedBatchIndicesSubsets.size(); ++subsetIndex)
    {
        int runningOffset = 0;
        for (int batchIdx : selectedBatchIndicesSubsets[subsetIndex])
        {
            // Append the pixel indices from the current batch.
            _GPUBatches[subsetIndex].insert(_GPUBatches[subsetIndex].end(),
                batches[batchIdx].begin(),
                batches[batchIdx].end());
            // For each ray (pixel) in the current batch, compute its start offset.
            for (int sampleCount : batchRaySampleAmount[batchIdx])
            {
                _GPUBatchesStartIndex[subsetIndex].push_back(runningOffset);
                runningOffset += sampleCount;
            }
        }
    }
}

// This function retrieves the full data from the GPU using compute shaders.
// It uses given vectors to decide which rays to sample and how much memory to allocate for the output.
// The output is stored in the _outputSSBO buffer, which is then mapped to a CPU-side vector.
// The function also handles the creation and deletion of the buffers as needed.
// @param cpuOutput: Vector to store the resulting samples from the GPU.
// @param subsetsMemory: Vector of the maximum required memory sizes in bytes to store the resulting samples of each subset.
// @param batchIndex: Index of the batch we want to retrieve data for.
// @param GPUBatches: Vector of vectors containing the pixel indices for each batch.
// @param GPUBatchesStartIndex: Vector of vectors containing the start indices in the write buffer for each ray in a batch.
// @param deleteBuffers: If true, the buffers will be deleted after use.
void VolumeRenderer::retrieveBatchFullData(std::vector<float>& cpuOutput, int batchIndex, bool deleteBuffers)
{
    //Create the buffers if needed
    if (!_GPUFullDataModeBuffersInitialized) {
        glGenBuffers(1, &_indicesSSBO);
        glGenBuffers(1, &_startIndexSSBO);
        glGenBuffers(1, &_outputDataSSBO);
        _GPUFullDataModeBuffersInitialized = true;
        qDebug() << "Created GPU buffers for full data mode";
    }

    // populate The buffers
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _indicesSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
        _GPUBatches[batchIndex].size() * sizeof(int),    // total reserved bytes for output.
        _GPUBatches[batchIndex].data(),                  // pointer to the data.
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _indicesSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _startIndexSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
        _GPUBatchesStartIndex[batchIndex].size() * sizeof(int),
        _GPUBatchesStartIndex[batchIndex].data(),
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _startIndexSSBO);

    //The write buffers
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _outputDataSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
        _subsetsMemory[batchIndex],
        nullptr,
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _outputDataSSBO);

    // Bind the program
    _fullDataSamplerComputeShader->bind();
    _backfacesTexture.bind(0);
    _fullDataSamplerComputeShader->setUniformValue("backFaces", 0);

    _frontfacesTexture.bind(1);
    _fullDataSamplerComputeShader->setUniformValue("frontFaces", 1);

    _volumeTexture.bind(2);
    _fullDataSamplerComputeShader->setUniformValue("volumeData", 2);

    mv::Vector3f volumeSize;
    mv::Vector3f invVolumeSize;
    if (_useCustomRenderSpace) {
        volumeSize = _renderSpace;
        invVolumeSize = mv::Vector3f(1.0f / _renderSpace.x, 1.0f / _renderSpace.y, 1.0f / _renderSpace.z);
    }
    else {
        volumeSize = _volumeSize;
        invVolumeSize = mv::Vector3f(1.0f / _volumeSize.x, 1.0f / _volumeSize.y, 1.0f / _volumeSize.z);
    }

    QVector3D atlasLayout(_volumeTextureSize.x / volumeSize.x, _volumeTextureSize.y / volumeSize.y, _volumeTextureSize.z / volumeSize.z);
    QVector3D invAtlasLayout(1.0f / atlasLayout.x(), 1.0f / atlasLayout.y(), 1.0f / atlasLayout.z());

    int bricksNeeded = (_volumeDataset->getComponentsPerVoxel() + 3) / 4;

    _fullDataSamplerComputeShader->setUniformValue("dataDimensions", QVector3D(volumeSize.x, volumeSize.y, volumeSize.z));
    _fullDataSamplerComputeShader->setUniformValue("invDataDimensions", QVector3D(invVolumeSize.x, invVolumeSize.y, invVolumeSize.z));
    _fullDataSamplerComputeShader->setUniformValue("atlasLayout", atlasLayout);
    _fullDataSamplerComputeShader->setUniformValue("invAtlasLayout", invAtlasLayout);
    _fullDataSamplerComputeShader->setUniformValue("voxelDimensions", _volumeDataset->getComponentsPerVoxel());
    _fullDataSamplerComputeShader->setUniformValue("invFaceTexSize", QVector2D(1.0f / _adjustedScreenSize.width(), 1.0f / _adjustedScreenSize.height()));

    _fullDataSamplerComputeShader->setUniformValue("stepSize", _stepSize);
    _fullDataSamplerComputeShader->setUniformValue("numIndices", static_cast<int>(_GPUBatches[batchIndex].size()));
    _fullDataSamplerComputeShader->setUniformValue("bricksNeeded", bricksNeeded);

    qDebug() << "Initialized compute shader with write memory size" << _subsetsMemory[batchIndex] / (1024 * 1024) << "MB";
    // Dispatch the compute shader, we launch one invocation per index;
    glDispatchCompute(_GPUBatches[batchIndex].size(), 1, 1);

    // Since the shader writes float values, we'll copy into a vector of floats.
    size_t numFloats = _subsetsMemory[batchIndex] / sizeof(float);
    cpuOutput.resize(numFloats);

    // Ensure that all writes to SSBOs are finished.
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    glFinish();

    // Bind the output buffers for reading, use glGetBufferSubData to copy the data directly.
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _outputDataSSBO);

    // Use glGetBufferSubData to copy the data directly.
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, _subsetsMemory[batchIndex], cpuOutput.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


    if (deleteBuffers)
    {
        // Delete the buffers after use
        glDeleteBuffers(1, &_indicesSSBO);
        glDeleteBuffers(1, &_startIndexSSBO);
        glDeleteBuffers(1, &_outputDataSSBO);
        _GPUFullDataModeBuffersInitialized = false;
        qDebug() << "Deleted GPU buffers for full data mode";
    }
}

// TODO : This function should be moved to a more appropriate location, as it is not specific to the VolumeRenderer class.
// Compute the unweighted mean of a std::vector<QVector2D>
QVector2D computeMean(const std::vector<QVector2D>& points,
    const std::vector<int>& indices) {
    QVector2D sum(0, 0);
    for (int idx : indices) sum += points[idx];
    return sum / float(indices.size());
}


// TODO : This function should be moved to a more appropriate location, as it is not specific to the VolumeRenderer class.
// Compute the weighted mean of a std::vector<QVector2D> given corresponding weight values.
QVector2D computeWeightedMean(const std::vector<QVector2D>& points,
    const std::vector<float>& weights,
    const std::vector<int>& indices) {
    QVector2D sum(0, 0);
    float weightSum = 0.0f;
    for (int idx : indices) {
        sum += points[idx] * weights[idx];
        weightSum += weights[idx];
    }
    return sum / (weightSum);
}

// This function renders the full data to the screen using the composite shader.
// It takes the GPU batches and their start indices, as well as the mean positions of the samples.
// The function also takes and updates the composite texture of the previous results as input, such that all previous batches are also rendered to the screen.
void VolumeRenderer::renderBatchToScreen(int batchIndex, uint32_t sampleDim, std::vector<float>& meanPositions)
{
    int width = _adjustedScreenSize.width();
    int height = _adjustedScreenSize.height();

    std::vector<int> mappingSampleStart(_GPUBatchesStartIndex[batchIndex].size() + 1); // Start index for each ray as if each sample takes one space (we multiply by 2 in the shader)
    for (size_t i = 0; i < mappingSampleStart.size(); i++) {
        mappingSampleStart[i] = (_GPUBatchesStartIndex[batchIndex][i]); // The GPU start index has all components for each sample, so we need to divide by the number of components to get the samplePos
    }
    mappingSampleStart[mappingSampleStart.size() - 1] = meanPositions.size() / 2; //Since the mappingSampleStart array keeps the indices for the sample amount and the meanPosition vector contains two floats per sample
    int numRays = _GPUBatchesStartIndex[batchIndex].size();

    std::vector<int> rayIDTextureData(_adjustedScreenSize.width() * _adjustedScreenSize.height(), -1);
    int rayID = 0;
    for (int i = 0; i < _GPUBatches[batchIndex].size(); i++) {
        int pixelIndex = _GPUBatches[batchIndex][i];
        rayIDTextureData[pixelIndex] = rayID;
        rayID++;
    }
    qDebug() << "shader vectors created.";
    mv::Texture2D rayIDTexture;
    rayIDTexture.create();
    rayIDTexture.bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, _adjustedScreenSize.width(), _adjustedScreenSize.height(), 0, GL_RED_INTEGER, GL_INT, rayIDTextureData.data());
    rayIDTexture.release();

    GLuint sampleMappingBuffer;
    glGenBuffers(1, &sampleMappingBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, sampleMappingBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
        mappingSampleStart.size() * sizeof(int),
        mappingSampleStart.data(),
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, sampleMappingBuffer);

    GLuint meanPositionsBuffer;
    glGenBuffers(1, &meanPositionsBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, meanPositionsBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
        meanPositions.size() * sizeof(float),
        meanPositions.data(),
        GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, meanPositionsBuffer);

    // Swap over to a different framebuffer that we can use to write the results to a texture instead of the screen.
    _framebuffer.bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    _framebuffer.setTexture(GL_COLOR_ATTACHMENT0, _prevFullCompositeTexture);
    //_framebuffer.setTexture(GL_DEPTH_ATTACHMENT, _depthTexture);

    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_FULL) {
        // --- Set up and bind the composite shader ---
        _fullDataCompositeShader.bind();

        // Bind our textures to the expected units.
        rayIDTexture.bind(0);
        _fullDataCompositeShader.uniform1i("rayIDTexture", 0);
        _tfTexture.bind(1);
        _fullDataCompositeShader.uniform1i("tfTexture", 1);

        _fullDataCompositeShader.uniform2f("invFaceTexSize", 1.0f / float(width), 1.0f / float(height));
        _fullDataCompositeShader.uniform2f("invTfTexSize", 1.0f / _tfDataset->getImageSize().width(), 1.0f / _tfDataset->getImageSize().height());
        _fullDataCompositeShader.uniform1f("stepSize", _stepSize);
        _fullDataCompositeShader.uniform1i("useClutterRemover", _useClutterRemover);

        // Render a full-screen quad to composite the current batch's results over prevFullCompositeTexture.
        drawDVRQuad(_fullDataCompositeShader);
    }
    else if (_renderMode == RenderMode::MaterialTransition_FULL) {
        // --- Set up and bind the full data material blending shader ---
        _fullDataMaterialTransitionShader.bind();

        // Bind our textures to the expected units.
        rayIDTexture.bind(0);
        _fullDataMaterialTransitionShader.uniform1i("rayIDTexture", 0);

        _materialTransitionTexture.bind(1);
        _fullDataMaterialTransitionShader.uniform1i("materialTexture", 1);

        _materialPositionTexture.bind(2);
        _fullDataMaterialTransitionShader.uniform1i("tfTexture", 2);

        _tempNNMaterialVolume.bind(3);
        _fullDataMaterialTransitionShader.uniform1i("materialVolumeData", 3);

        _frontfacesTexture.bind(4);
        _fullDataMaterialTransitionShader.uniform1i("frontFaces", 4);

        _backfacesTexture.bind(5);
        _fullDataMaterialTransitionShader.uniform1i("backFaces", 5);

        // Set required uniforms
        _fullDataMaterialTransitionShader.uniform2f("invFaceTexSize", 1.0f / float(width), 1.0f / float(height));
        _fullDataMaterialTransitionShader.uniform2f("invTfTexSize", 1.0f / _materialPositionDataset->getImageSize().width(), 1.0f / _materialPositionDataset->getImageSize().height());
        _fullDataMaterialTransitionShader.uniform2f("invMatTexSize", 1.0f / _materialTransitionDataset->getImageSize().width(), 1.0f / _materialTransitionDataset->getImageSize().height());
        _fullDataMaterialTransitionShader.uniform1i("numRays", numRays);
        _fullDataMaterialTransitionShader.uniform1f("stepSize", _stepSize);
        _fullDataMaterialTransitionShader.uniform3f("dataDimensions", _volumeSize.x, _volumeSize.y, _volumeSize.z);

        // Render a full-screen quad to composite the current batch's results over prevFullCompositeTexture.
        drawDVRQuad(_fullDataMaterialTransitionShader);

    }
    else {
        qCritical() << "Unsupported render mode for full data rendering.";
        return;
    }

    _framebuffer.release();

    qDebug() << "Composite full data rendered into composite texture.";

    // Clean up temporary GPU buffers.
    glDeleteBuffers(1, &sampleMappingBuffer);
    glDeleteBuffers(1, &meanPositionsBuffer);

    // Finally, render the updated composite texture to the screen(the default framebuffer).
    glBindFramebuffer(GL_FRAMEBUFFER, _defaultFramebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderTexture(_prevFullCompositeTexture);

}

// This function computes the mean of the nearest neighbours for a given set of neighbours.
// @param neighbours: A vector of pairs containing the distance and label of each neighbour.
// @param k: The number of neighbours to consider.
// @param positionData: A vector containing the 2D positions of the neighbours.
QVector2D VolumeRenderer::ComputeMeanOfNN(const std::vector<std::pair<float, int64_t>>& neighbors, int k, const std::vector<float>& positionData) {
    float epsilon = 1.0f;  // To avoid division by zero and limit the impact of very close neighbours.
    float clusterSlack = 0.1f; // Slack for cluster thresholding, can be adjusted based on the dataset.

    std::vector<float> weights(k, 0.0f);
    std::vector<QVector2D> candidatePositions(k, QVector2D(0.0f, 0.0f));
    int j = 0;
    for (const auto& entry : neighbors) {
        weights[j] = (1.0f / (entry.first + epsilon));  // Inverse distance is used as weight
        float posX = positionData[entry.second * 2];
        float posY = positionData[entry.second * 2 + 1];
        candidatePositions[j] = QVector2D(posX, posY);
        j++;
    }

    // 2) Optionally extract largest cluster via MST + relative-threshold 
    std::vector<int> chosenIndices;
    chosenIndices.reserve(k);

    if (useLargestCluster && k > 1) {
        // 2a) Build full distance matrix
        std::vector<float> distanceMatrix(k * k);
        for (int a = 0; a < k; ++a) {
            distanceMatrix[a * k + a] = 0.0f;
            for (int b = a + 1; b < k; ++b) {
                float dx = candidatePositions[a].x() - candidatePositions[b].x();
                float dy = candidatePositions[a].y() - candidatePositions[b].y();
                float d = std::sqrt(dx * dx + dy * dy);
                distanceMatrix[a * k + b] = d;
                distanceMatrix[b * k + a] = d;
            }
        }

        // 2b) Prim’s MST: collect k-1 smallest edges
        std::vector<bool>   inTree(k, false);
        std::vector<float>  minEdgeToTree(k, FLT_MAX);
        std::vector<int>    mstParent(k, -1);
        struct MstEdge { float weight; int u, v; };
        std::vector<MstEdge> mstEdges;
        mstEdges.reserve(k - 1);

        inTree[0] = true;
        for (int v = 1; v < k; ++v) {
            minEdgeToTree[v] = distanceMatrix[v];
            mstParent[v] = 0;
        }

        for (int e = 0; e < k - 1; ++e) {
            // pick frontier vertex with smallest connecting edge
            int bestV = -1;
            float bestW = FLT_MAX;
            for (int v = 0; v < k; ++v) {
                if (!inTree[v] && minEdgeToTree[v] < bestW) {
                    bestW = minEdgeToTree[v];
                    bestV = v;
                }
            }
            mstEdges.push_back({ bestW, mstParent[bestV], bestV });
            inTree[bestV] = true;

            // update frontier
            for (int v = 0; v < k; ++v) {
                float w = distanceMatrix[bestV * k + v];
                if (!inTree[v] && w < minEdgeToTree[v]) {
                    minEdgeToTree[v] = w;
                    mstParent[v] = bestV;
                }
            }
        }

        // 2c) Determine threshold: smallest MST edge + slack
        float minMstWeight = FLT_MAX;
        for (auto& e : mstEdges) {
            minMstWeight = std::min(minMstWeight, e.weight);
        }
        float threshold = minMstWeight + clusterSlack;

        // 2d) Cut edges > threshold and form clusters via union-find
        UnionFind uf(k);
        for (auto& e : mstEdges) {
            if (e.weight <= threshold) {
                uf.unify(e.u, e.v);
            }
        }

        // 2e) Group vertices by root to get clusters
        std::unordered_map<int, std::vector<int>> clusters;
        clusters.reserve(k);
        for (int v = 0; v < k; ++v) {
            clusters[uf.findRoot(v)].push_back(v);
        }

        // 2f) Pick the largest cluster
        int maxSize = 0;
        for (auto& kv : clusters) {
            int sz = int(kv.second.size());
            if (sz > maxSize) {
                maxSize = sz;
                chosenIndices = kv.second;
            }
        }
    }
    else {
        // use all neighbors if clustering disabled
        chosenIndices.resize(k);
        std::iota(chosenIndices.begin(), chosenIndices.end(), 0);
    }

    // 3) Compute mean position (weighted or unweighted)
    QVector2D meanPos;
    if (useWeightedMean)
        meanPos = computeWeightedMean(candidatePositions, weights, chosenIndices);
    else
        meanPos = computeMean(candidatePositions, chosenIndices);
    return meanPos;
}



void VolumeRenderer::updateRenderModeParameters()
{
    // Get the screen dimensions and allocate arrays to read the front and back face textures.
    int screenWidth = _adjustedScreenSize.width();
    int screenHeight = _adjustedScreenSize.height();

    std::vector<float> frontfacesData(screenWidth * screenHeight * 3);
    std::vector<float> backfacesData(screenWidth * screenHeight * 3);
    getFacesTextureData(frontfacesData, backfacesData);
    qDebug() << "Front and backfaces data retrieved.";

    // Create the GPU full data batches. ---
    getGPUFullDataModeBatches(frontfacesData, backfacesData);
    // Initialize the previous composite texture, this texture will hold the cumulative composite result.
    std::vector<float> emptyTextureData(screenWidth * screenHeight * 3, 0.0f);

    _prevFullCompositeTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, screenWidth, screenHeight, 0, GL_RGB, GL_FLOAT, emptyTextureData.data());
    _prevFullCompositeTexture.release();
    qDebug() << "Previous composite texture initialized.";

    if (_renderMode == RenderMode::MaterialTransition_FULL) {
        // --- Create a temporary texture to hold the material volume data that is used to clean up noisy material transitions ---
        _tempNNMaterialVolume.create();
        _tempNNMaterialVolume.bind();
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        _tempNNMaterialVolume.release();

        // Load the material position dataset into the texture.
        loadNNVolumeToTexture(_tempNNMaterialVolume, _textureData, _materialPositionImage, _materialPositionDataset->getImageSize().width(), _volumeSize, _volumeDataset->getNumberOfVoxels(), true);
    }
}

void VolumeRenderer::renderFullData()
{
    // Check available GPU memory for the batch transfer.
    size_t availableMemoryInBytes = _fullGPUMemorySize - _fullDataMemorySize - 100000; // Reserve ~100MB for other data.
    if (availableMemoryInBytes < 0) {
        qCritical() << "Not enough GPU memory available for the GPU-CPU batch transfer.";
        return;
    }

    // Make sure the ANN (e.g. hnswlib) is prepared for the dataset.
    if (!_ANNAlgorithmTrained) {
        prepareANN();
        _ANNAlgorithmTrained = true;
        qDebug() << "ANN algorithm trained for full data mode.";
    }

    // Initialize the GPU full data mode parameters if not already done.
    if (_fullDataModeBatch == -1) {
        qDebug() << "Available GPU memory for batch transfer:" << availableMemoryInBytes / (1024 * 1024) << "MB";
        qDebug() << "Rendering composite full data...";

        updateRenderModeParameters();
        _fullDataModeBatch = 0;
    }

    std::vector<float> cpuOutput;

    retrieveBatchFullData(cpuOutput, _fullDataModeBatch, true);

    // Retrieve the reduced 2D position data (e.g. from a dimension reduction dataset), they are needed for following computation ---
    int pointAmount = _volumeDataset->getNumberOfVoxels() * 2; // two floats per voxel.
    std::vector<float> positionData(pointAmount);
    _reducedPosDataset->populateDataForDimensions(positionData, std::vector<int>{0, 1});
    normalizePositionData(positionData);

    // Run approximate nearest-neighbour search on the retrieved CPU data.
    uint32_t sampleDim = _volumeDataset->getComponentsPerVoxel();
    int64_t numQueries = static_cast<int64_t>(cpuOutput.size() / sampleDim);
    std::vector<float> meanPositions(numQueries * 2);

    int k = 1; // Number of nearest neighbours to consider for the mean position computation.
    if (_useShading) { // I just use the same button since it is not used anyway
        k = 9;
    }
    bool useWeightedMean = true;  // change to "true" if you need weighting.
    batchSearch(cpuOutput, positionData, sampleDim, k, useWeightedMean, meanPositions);

    cpuOutput.clear();  // Free memory immediately.
    qDebug() << "Approximate lower dimensional positions estimated" << _fullDataModeBatch;

    // Composite this batch’s result over the previous composite and update the texture.
    renderBatchToScreen( _fullDataModeBatch, sampleDim, meanPositions);
    qDebug() << "Rendered batch" << _fullDataModeBatch << "to composite texture.";
    if (_fullDataModeBatch == _GPUBatches.size() - 1) {
        _fullDataModeBatch = -1;

        // clean up the temporary texture used for the material volume.
        _tempNNMaterialVolume.destroy();
        qDebug() << "Composite full rendering completed.";
    }
    else {
        _fullDataModeBatch++;
    }
}

void VolumeRenderer::renderComposite2DPos()
{
    setDefaultRenderSettings();

    // Bind the framebuffer and attach the adapted screen size texture
    _framebuffer.bind();
    _framebuffer.setTexture(GL_COLOR_ATTACHMENT0, _adaptedScreenSizeTexture);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up and bind the 2D composite shader
    _2DCompositeShader.bind();
    _backfacesTexture.bind(0);
    _2DCompositeShader.uniform1i("backFaces", 0);

    _frontfacesTexture.bind(1);
    _2DCompositeShader.uniform1i("frontFaces", 1);

    _volumeTexture.bind(2);
    _2DCompositeShader.uniform1i("volumeData", 2);

    _tfTexture.bind(3);
    _2DCompositeShader.uniform1i("tfTexture", 3);

    _2DCompositeShader.uniform1f("stepSize", _stepSize);

    mv::Vector3f volumeSize;
    mv::Vector3f invVolumeSize;
    if (_useCustomRenderSpace) {
        volumeSize = _renderSpace;
        invVolumeSize = mv::Vector3f(1.0f / _renderSpace.x, 1.0f / _renderSpace.y, 1.0f / _renderSpace.z);
    }
    else {
        volumeSize = _volumeSize;
        invVolumeSize = mv::Vector3f(1.0f / _volumeSize.x, 1.0f / _volumeSize.y, 1.0f / _volumeSize.z);
    }

    _2DCompositeShader.uniform3fv("dimensions", 1, &volumeSize);
    _2DCompositeShader.uniform3fv("invDimensions", 1, &invVolumeSize);
    _2DCompositeShader.uniform2f("invFaceTexSize", 1.0f / _adjustedScreenSize.width(), 1.0f / _adjustedScreenSize.height());
    _2DCompositeShader.uniform2f("invTfTexSize", 1.0f / _tfDataset->getImageSize().width(), 1.0f / _tfDataset->getImageSize().height());

    drawDVRQuad(_2DCompositeShader);

    _framebuffer.release();

    // Now render the adapted screen size texture to the default framebuffer (the screen)
    glBindFramebuffer(GL_FRAMEBUFFER, _defaultFramebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderTexture(_adaptedScreenSizeTexture);

    // Restore depth clear value
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);
}

void VolumeRenderer::renderCompositeColor()
{
    setDefaultRenderSettings();

    // Bind the framebuffer and attach the adapted screen size texture
    _framebuffer.bind();
    _framebuffer.setTexture(GL_COLOR_ATTACHMENT0, _adaptedScreenSizeTexture);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Optionally attach depth if needed:
    // _framebuffer.setTexture(GL_DEPTH_ATTACHMENT, _depthTexture);

    // Set up and bind the color composite shader
    _colorCompositeShader.bind();
    _backfacesTexture.bind(0);
    _colorCompositeShader.uniform1i("backFaces", 0);

    _frontfacesTexture.bind(1);
    _colorCompositeShader.uniform1i("frontFaces", 1);

    _volumeTexture.bind(2);
    _colorCompositeShader.uniform1i("volumeData", 2);

    _colorCompositeShader.uniform1f("stepSize", _stepSize);

    mv::Vector3f volumeSize;
    mv::Vector3f invVolumeSize;
    mv::Vector3f dimesnionVolumeRatio;
    if (_useCustomRenderSpace) {
        volumeSize = _renderSpace;
        invVolumeSize = mv::Vector3f(1.0f / _renderSpace.x, 1.0f / _renderSpace.y, 1.0f / _renderSpace.z);
        dimesnionVolumeRatio = mv::Vector3f(_renderSpace.x / _volumeSize.x, _renderSpace.y / _volumeSize.y, _renderSpace.z / _volumeSize.z);
    }
    else {
        volumeSize = _volumeSize;
        invVolumeSize = mv::Vector3f(1.0f / _volumeSize.x, 1.0f / _volumeSize.y, 1.0f / _volumeSize.z);
        dimesnionVolumeRatio = mv::Vector3f(1.0f, 1.0f, 1.0f); // No custom render space, so ratio is 1:1
    }

    _colorCompositeShader.uniform3fv("dimensions", 1, &volumeSize);
    _colorCompositeShader.uniform3fv("invDimensions", 1, &invVolumeSize);
    _colorCompositeShader.uniform2f("invFaceTexSize", 1.0f / _adjustedScreenSize.width(), 1.0f / _adjustedScreenSize.height());
    _colorCompositeShader.uniform2f("invTfTexSize", 1.0f / _tfDataset->getImageSize().width(), 1.0f / _tfDataset->getImageSize().height());

    //_colorCompositeShader.uniform3fv("dimensionVolumeRatio", 1, &dimesnionVolumeRatio);

    drawDVRQuad(_colorCompositeShader);

    _framebuffer.release();

    // Now render the adapted screen size texture to the default framebuffer (the screen)
    glBindFramebuffer(GL_FRAMEBUFFER, _defaultFramebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderTexture(_adaptedScreenSizeTexture);

    // Restore depth clear value
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);
}


// Render using a standard MIP algorithm on a 1D slice of the volume
void VolumeRenderer::render1DMip()
{
    setDefaultRenderSettings();

    // Bind the framebuffer and attach the adapted screen size texture
    _framebuffer.bind();
    _framebuffer.setTexture(GL_COLOR_ATTACHMENT0, _adaptedScreenSizeTexture);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up and bind the 1D MIP shader
    _1DMipShader.bind();
    _backfacesTexture.bind(0);
    _1DMipShader.uniform1i("backFaces", 0);

    _frontfacesTexture.bind(1);
    _1DMipShader.uniform1i("frontFaces", 1);

    _volumeTexture.bind(2);
    _1DMipShader.uniform1i("volumeData", 2);

    _1DMipShader.uniform1f("stepSize", _stepSize);
    _1DMipShader.uniform1f("volumeMaxValue", _scalarVolumeDataRange.second);
    _1DMipShader.uniform1i("chosenDim", _mipDimension);

    mv::Vector3f volumeSize;
    mv::Vector3f invVolumeSize;
    if (_useCustomRenderSpace) {
        volumeSize = _renderSpace;
        invVolumeSize = mv::Vector3f(1.0f / _renderSpace.x, 1.0f / _renderSpace.y, 1.0f / _renderSpace.z);
    }
    else {
        volumeSize = _volumeSize;
        invVolumeSize = mv::Vector3f(1.0f / _volumeSize.x, 1.0f / _volumeSize.y, 1.0f / _volumeSize.z);
    }

    _1DMipShader.uniform3fv("dimensions", 1, &volumeSize);
    _1DMipShader.uniform3fv("invDimensions", 1, &invVolumeSize);
    _1DMipShader.uniform2f("invFaceTexSize", 1.0f / _adjustedScreenSize.width(), 1.0f / _adjustedScreenSize.height());

    drawDVRQuad(_1DMipShader);

    _framebuffer.release();

    // Now render the adapted screen size texture to the default framebuffer (the screen)
    glBindFramebuffer(GL_FRAMEBUFFER, _defaultFramebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderTexture(_adaptedScreenSizeTexture);

    // Restore depth clear value
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);
}

void VolumeRenderer::renderMaterialTransition2D()
{
    setDefaultRenderSettings();

    // Bind the framebuffer and attach the adapted screen size texture
    _framebuffer.bind();
    _framebuffer.setTexture(GL_COLOR_ATTACHMENT0, _adaptedScreenSizeTexture);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up and bind the shader
    _materialTransition2DShader.bind();
    _backfacesTexture.bind(0);
    _materialTransition2DShader.uniform1i("backFaces", 0);

    _frontfacesTexture.bind(1);
    _materialTransition2DShader.uniform1i("frontFaces", 1);

    _volumeTexture.bind(2);
    _materialTransition2DShader.uniform1i("volumeData", 2);

    _materialPositionTexture.bind(3);
    _materialTransition2DShader.uniform1i("tfTexture", 3);

    _materialTransitionTexture.bind(4);
    _materialTransition2DShader.uniform1i("materialTexture", 4);

    _materialTransition2DShader.uniform1f("stepSize", _stepSize);

    _materialTransition2DShader.uniform1i("useShading", _useShading);
    _materialTransition2DShader.uniform1f("useClutterRemover", _useClutterRemover);
    _materialTransition2DShader.uniform3fv("camPos", 1, &_cameraPos);
    _materialTransition2DShader.uniform3fv("lightPos", 1, &_cameraPos);

    mv::Vector3f volumeSize;
    mv::Vector3f invVolumeSize;
    if (_useCustomRenderSpace) {
        volumeSize = _renderSpace;
        invVolumeSize = mv::Vector3f(1.0f / _renderSpace.x, 1.0f / _renderSpace.y, 1.0f / _renderSpace.z);
    }
    else {
        volumeSize = _volumeSize;
        invVolumeSize = mv::Vector3f(1.0f / _volumeSize.x, 1.0f / _volumeSize.y, 1.0f / _volumeSize.z);
    }

    _materialTransition2DShader.uniform3fv("dimensions", 1, &volumeSize);
    _materialTransition2DShader.uniform3fv("invDimensions", 1, &invVolumeSize);
    _materialTransition2DShader.uniform2f("invFaceTexSize", 1.0f / _adjustedScreenSize.width(), 1.0f / _adjustedScreenSize.height());
    _materialTransition2DShader.uniform2f("invTfTexSize", 1.0f / _materialPositionDataset->getImageSize().width(), 1.0f / _materialPositionDataset->getImageSize().height());
    _materialTransition2DShader.uniform2f("invMatTexSize", 1.0f / _materialTransitionDataset->getImageSize().width(), 1.0f / _materialTransitionDataset->getImageSize().height());

    drawDVRQuad(_materialTransition2DShader);

    _framebuffer.release();

    // Now render the adapted screen size texture to the default framebuffer (the screen)
    glBindFramebuffer(GL_FRAMEBUFFER, _defaultFramebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderTexture(_adaptedScreenSizeTexture);

    // Restore depth clear value
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);
}

void VolumeRenderer::renderNNMaterialTransition()
{
    setDefaultRenderSettings();

    // Bind the framebuffer and attach the adapted screen size texture
    _framebuffer.bind();
    _framebuffer.setTexture(GL_COLOR_ATTACHMENT0, _adaptedScreenSizeTexture);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up and bind the shader
    _nnMaterialTransitionShader.bind();
    _backfacesTexture.bind(0);
    _nnMaterialTransitionShader.uniform1i("backFaces", 0);

    _frontfacesTexture.bind(1);
    _nnMaterialTransitionShader.uniform1i("frontFaces", 1);

    _volumeTexture.bind(2);
    _nnMaterialTransitionShader.uniform1i("volumeData", 2);

    _materialTransitionTexture.bind(3);
    _nnMaterialTransitionShader.uniform1i("materialTexture", 3);

    _nnMaterialTransitionShader.uniform1f("stepSize", _stepSize);
    _nnMaterialTransitionShader.uniform1f("useClutterRemover", _useClutterRemover);
    _nnMaterialTransitionShader.uniform1i("useShading", _useShading);
    _nnMaterialTransitionShader.uniform3fv("camPos", 1, &_cameraPos);
    _nnMaterialTransitionShader.uniform3fv("lightPos", 1, &_cameraPos);

    mv::Vector3f volumeSize;
    mv::Vector3f invVolumeSize;
    if (_useCustomRenderSpace) {
        volumeSize = _renderSpace;
        invVolumeSize = mv::Vector3f(1.0f / _renderSpace.x, 1.0f / _renderSpace.y, 1.0f / _renderSpace.z);
    }
    else {
        volumeSize = _volumeSize;
        invVolumeSize = mv::Vector3f(1.0f / _volumeSize.x, 1.0f / _volumeSize.y, 1.0f / _volumeSize.z);
    }

    _nnMaterialTransitionShader.uniform3fv("dimensions", 1, &volumeSize);
    _nnMaterialTransitionShader.uniform3fv("invDimensions", 1, &invVolumeSize);
    _nnMaterialTransitionShader.uniform2f("invFaceTexSize", 1.0f / _adjustedScreenSize.width(), 1.0f / _adjustedScreenSize.height());
    _nnMaterialTransitionShader.uniform2f("invMatTexSize", 1.0f / _materialTransitionDataset->getImageSize().width(), 1.0f / _materialTransitionDataset->getImageSize().height());

    drawDVRQuad(_nnMaterialTransitionShader);

    _framebuffer.release();

    // Now render the adapted screen size texture to the default framebuffer (the screen)
    glBindFramebuffer(GL_FRAMEBUFFER, _defaultFramebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderTexture(_adaptedScreenSizeTexture);

    // Restore depth clear value
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);
}

void VolumeRenderer::renderAltNNMaterialTransition()
{
    setDefaultRenderSettings();

    // Bind the framebuffer and attach the adapted screen size texture
    _framebuffer.bind();
    _framebuffer.setTexture(GL_COLOR_ATTACHMENT0, _adaptedScreenSizeTexture);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set up and bind the shader
    _altNNMaterialTransitionShader.bind();
    _backfacesTexture.bind(0);
    _altNNMaterialTransitionShader.uniform1i("backFaces", 0);

    _frontfacesTexture.bind(1);
    _altNNMaterialTransitionShader.uniform1i("frontFaces", 1);

    _volumeTexture.bind(2);
    _altNNMaterialTransitionShader.uniform1i("volumeData", 2);

    _materialTransitionTexture.bind(3);
    _altNNMaterialTransitionShader.uniform1i("materialTexture", 3);

    _altNNMaterialTransitionShader.uniform1i("useShading", _useShading);
    _altNNMaterialTransitionShader.uniform3fv("camPos", 1, &_cameraPos);
    _altNNMaterialTransitionShader.uniform3fv("lightPos", 1, &_cameraPos);

    mv::Vector3f volumeSize;
    mv::Vector3f invVolumeSize;
    if (_useCustomRenderSpace) {
        volumeSize = _renderSpace;
        invVolumeSize = mv::Vector3f(1.0f / _renderSpace.x, 1.0f / _renderSpace.y, 1.0f / _renderSpace.z);
    }
    else {
        volumeSize = _volumeSize;
        invVolumeSize = mv::Vector3f(1.0f / _volumeSize.x, 1.0f / _volumeSize.y, 1.0f / _volumeSize.z);
    }

    _altNNMaterialTransitionShader.uniform3fv("dimensions", 1, &volumeSize);
    _altNNMaterialTransitionShader.uniform3fv("invDimensions", 1, &invVolumeSize);
    _altNNMaterialTransitionShader.uniform2f("invFaceTexSize", 1.0f / _adjustedScreenSize.width(), 1.0f / _adjustedScreenSize.height());
    _altNNMaterialTransitionShader.uniform2f("invMatTexSize", 1.0f / _materialTransitionDataset->getImageSize().width(), 1.0f / _materialTransitionDataset->getImageSize().height());

    drawDVRQuad(_altNNMaterialTransitionShader);

    _framebuffer.release();

    // Now render the adapted screen size texture to the default framebuffer (the screen)
    glBindFramebuffer(GL_FRAMEBUFFER, _defaultFramebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderTexture(_adaptedScreenSizeTexture);

    // Restore depth clear value
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);
}

void VolumeRenderer::setDefaultRenderSettings()
{
    glClearDepth(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);
}


void VolumeRenderer::render()
{
    //These methods update the perquisites needed for any of the rendering methods
    updateMatrices();
    renderDirections();

    glBindFramebuffer(GL_FRAMEBUFFER, _defaultFramebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Check if all datasets are valid before rendering
    if (_volumeDataset.isValid() && _reducedPosDataset.isValid() && _tfDataset.isValid() && _materialPositionDataset.isValid() && _materialTransitionDataset.isValid()) {
        if (_dataSettingsChanged) {
            updataDataTexture();
            _dataSettingsChanged = false;
        }
        if (_renderMode == RenderMode::MaterialTransition_FULL || _renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_FULL)
            renderFullData();
        else if (_renderMode == RenderMode::MaterialTransition_2D)
            renderMaterialTransition2D();
        else if (_renderMode == RenderMode::NN_MaterialTransition)
            renderNNMaterialTransition();
        else if (_renderMode == RenderMode::Alt_NN_MaterialTransition)
            renderAltNNMaterialTransition();
        else if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_2D_POS)
            renderComposite2DPos();
        else if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_COLOR || _renderMode == RenderMode::NN_MULTIDIMENSIONAL_COMPOSITE)
            renderCompositeColor();
        else if (_renderMode == RenderMode::MIP)
            render1DMip();
        else {
            qCritical() << "Missing data for rendering";
        }
    }
    else {
        renderTexture(_frontfacesTexture);
    }
}

void VolumeRenderer::renderTexture(mv::Texture2D& texture)
{
    _textureShader.bind();

    texture.bind(0);
    _textureShader.uniform1i("tex", 0);
    drawDVRRender(_textureShader);
}

void VolumeRenderer::destroy()
{
    _vao.destroy();
    _vboCube.destroy();
    _iboCube.destroy();
    _surfaceShader.destroy();
    _textureShader.destroy();
}

