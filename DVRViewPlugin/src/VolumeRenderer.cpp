#include "VolumeRenderer.h"
#include <QImage>
#include <random>
#include <QOpenGLWidget>
#include <queue>
#include <algorithm>
#include <numeric>

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

    _tfRectangleDataTexture.create();
    _tfRectangleDataTexture.bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

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


    // Initialize buffers needed for the empty space skipping rendercubes
    glGenBuffers(1, &_renderCubePositionsBufferID);
    glBindBuffer(GL_TEXTURE_BUFFER, _renderCubePositionsBufferID);
    glBufferData(GL_TEXTURE_BUFFER, 1, NULL, GL_DYNAMIC_DRAW); // Size will be set later
    glGenTextures(1, &_renderCubePositionsTexID);
    glBindTexture(GL_TEXTURE_BUFFER, _renderCubePositionsTexID);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, _renderCubePositionsBufferID);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glGenBuffers(1, &_renderCubeOccupancyBufferID);
    glBindBuffer(GL_TEXTURE_BUFFER, _renderCubeOccupancyBufferID);
    glBufferData(GL_TEXTURE_BUFFER, 1, NULL, GL_DYNAMIC_DRAW); // Size will be set later
    glGenTextures(1, &_renderCubeOccupancyTexID);
    glBindTexture(GL_TEXTURE_BUFFER, _renderCubeOccupancyTexID);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, _renderCubeOccupancyBufferID);
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
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, edgeTableSSBO); // Binding 0 matches the shader

    // Create and bind the triTable buffer
    glGenBuffers(1, &triTableSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, triTableSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, triTableSize, triTable, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, triTableSSBO); // Binding 1 matches the shader

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
    _backfacesTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, renderSize.width(), renderSize.height(), 0, GL_RGB, GL_FLOAT, nullptr);

    _frontfacesTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, renderSize.width(), renderSize.height(), 0, GL_RGB, GL_FLOAT, nullptr);

    _prevFullCompositeTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, renderSize.width(), renderSize.height(), 0, GL_RGB, GL_FLOAT, nullptr);

    _depthTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, renderSize.width(), renderSize.height(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    _screenSize = renderSize;

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

    _tfSumedAreaTable = std::vector<float>(dataSize / 4);
    //Calculate the sumed area table of the alpha values in the transfer function
    for (int x = 0; x < textureDims.width(); x++)
    {
        for (int y = 0; y < textureDims.height() - 1; y++) {
            int i = x + y * textureDims.width();
            int reverseYIndex = x + (textureDims.height() - 2 - y) * textureDims.width();
            _tfSumedAreaTable[i] = _tfImage[reverseYIndex * 4 + 3]; // The alpha value of the current pixel

            if (x != 0)
                _tfSumedAreaTable[i] += _tfSumedAreaTable[i - 1];
            if (y != 0)
                _tfSumedAreaTable[i] += _tfSumedAreaTable[i - textureDims.width()];
            if (x != 0 && y != 0)
                _tfSumedAreaTable[i] -= _tfSumedAreaTable[i - textureDims.width() - 1];
        }
    }

    //for (int x = 0; x < textureDims.width(); x++)
    //{
    //    for (int y = textureDims.height() - 2; y >= 0; y--) {
    //        int i = x + y * textureDims.width();
    //        _tfSumedAreaTable[i] = _tfImage[i * 4 + 3]; // The alpha value of the current pixel

    //        if (x != 0)
    //            _tfSumedAreaTable[i] += _tfSumedAreaTable[i - 1];
    //        if (y != textureDims.height() - 2)
    //            _tfSumedAreaTable[i] += _tfSumedAreaTable[i + textureDims.width()];
    //        if (x != 0 && y != textureDims.height() - 2)
    //            _tfSumedAreaTable[i] -= _tfSumedAreaTable[i + textureDims.width() - 1];
    //    }
    //}


    _tfRectangleDataTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, textureDims.width(), textureDims.height() - 1, 0, GL_RED, GL_FLOAT, _tfSumedAreaTable.data());
    _tfRectangleDataTexture.release();

    // In these rendermodes the new dataset will impact the visualization and thus needs to be updated now 
    if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_COLOR || _renderMode == RenderMode::NN_MULTIDIMENSIONAL_COMPOSITE || _renderMode == RenderMode::NN_MaterialTransition || _renderMode == RenderMode::Alt_NN_MaterialTransition || _renderMode == RenderMode::Smooth_NN_MaterialTransition)
        updataDataTexture();
}

void VolumeRenderer::setReducedPosData(const mv::Dataset<Points>& reducedPosData)
{
    _reducedPosDataset = reducedPosData;

    // In these rendermodes the new dataset will impact the visualization and thus needs to be updated now 
    if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_2D_POS || _renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_COLOR || _renderMode == RenderMode::NN_MULTIDIMENSIONAL_COMPOSITE || _renderMode == RenderMode::NN_MaterialTransition || _renderMode == RenderMode::Alt_NN_MaterialTransition || _renderMode == RenderMode::Smooth_NN_MaterialTransition)
        updataDataTexture();
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

void VolumeRenderer::updateAuxilairySmoothNNTextures()
{
    int pointAmount = _volumeDataset->getNumberOfVoxels();
    _textureData = std::vector<float>(pointAmount * 4);
}

//void VolumeRenderer::updateRenderCubes()
//{
//    if (_volumeDataset.isValid() && _tfDataset.isValid() && _reducedPosDataset.isValid())
//    {
//        if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_FULL || _renderMode == RenderMode::MaterialTransition_FULL)
//        {
//            _renderCubesUpdated = true;
//            updateRenderCubes2DCoords(); // TODO add a better method for this
//        }
//        else if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_2D_POS || _renderMode == RenderMode::MaterialTransition_2D || _renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_COLOR || _renderMode == RenderMode::NN_MULTIDIMENSIONAL_COMPOSITE || _renderMode == RenderMode::NN_MaterialTransition || _renderMode == RenderMode::Alt_NN_MaterialTransition || _renderMode == RenderMode::Smooth_NN_MaterialTransition)
//        {
//            _renderCubesUpdated = true;
//            updateRenderCubes2DCoords();
//        }
//        else if (_renderMode == RenderMode::MIP)
//        {
//            //All areas of the volume are important in a MIP render so we don't need to render cubes
//            _renderCubesUpdated = true;
//        }
//        else
//            qCritical() << "Unknown render mode";
//    }
//    else
//        qCritical() << "No volume data set, transfer function or position data set";
//
//}

//void VolumeRenderer::updateRenderCubes2DCoords()
//{
//    mv::Vector3f relativeBlockSize = mv::Vector3f(_renderCubeSize / _volumeSize.x, _renderCubeSize / _volumeSize.y, _renderCubeSize / _volumeSize.z);
//
//    int nx = std::ceil(1.0f / relativeBlockSize.x);
//    int ny = std::ceil(1.0f / relativeBlockSize.y);
//    int nz = std::ceil(1.0f / relativeBlockSize.z);
//
//    std::vector<mv::Vector3f> positions;
//    std::vector<float> occupancyValues;
//
//    for (int x = 0; x < nx; ++x)
//    {
//        for (int y = 0; y < ny; ++y)
//        {
//            for (int z = 0; z < nz; ++z)
//            {
//                mv::Vector3f posIndex = mv::Vector3f(x, y, z);
//                positions.push_back(posIndex);
//
//                mv::Vector2f topleft = mv::Vector2f(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
//                mv::Vector2f bottomRight = mv::Vector2f(std::numeric_limits<float>::min(), std::numeric_limits<float>::min());
//
//                // clamp the start and end points to the volume dimensions so they don't go out of bounds
//                mv::Vector3f startPoint = mv::Vector3f(
//                    std::max(0.0f, relativeBlockSize.x * posIndex.x * _volumeSize.x - 1.0f),
//                    std::max(0.0f, relativeBlockSize.y * posIndex.y * _volumeSize.y - 1.0f),
//                    std::max(0.0f, relativeBlockSize.z * posIndex.z * _volumeSize.z - 1.0f)
//                );
//
//                mv::Vector3f endPoint = mv::Vector3f(
//                    std::min(_volumeSize.x, relativeBlockSize.x * (posIndex.x + 1.0f) * _volumeSize.x + 1.0f),
//                    std::min(_volumeSize.y, relativeBlockSize.y * (posIndex.y + 1.0f) * _volumeSize.y + 1.0f),
//                    std::min(_volumeSize.z, relativeBlockSize.z * (posIndex.z + 1.0f) * _volumeSize.z + 1.0f)
//                );
//
//                for (int x1 = startPoint.x; x1 < endPoint.x; ++x1)
//                {
//                    for (int y1 = startPoint.y; y1 < endPoint.y; ++y1)
//                    {
//                        for (int z1 = startPoint.z; z1 < endPoint.z; ++z1)
//                        {
//                            int index = x1 + y1 * _volumeSize.x + z1 * _volumeSize.x * _volumeSize.y;
//                            float xvalue = _textureData[index * 2];
//                            float yvalue = _textureData[index * 2 + 1];
//
//                            if (xvalue < topleft.x)
//                                topleft.x = xvalue;
//                            if (xvalue > bottomRight.x)
//                                bottomRight.x = xvalue;
//
//                            if (yvalue < topleft.y)
//                                topleft.y = yvalue;
//                            if (yvalue > bottomRight.y)
//                                bottomRight.y = yvalue;
//                        }
//                    }
//                }
//                occupancyValues.push_back(topleft.x);
//                occupancyValues.push_back(topleft.y);
//                occupancyValues.push_back(bottomRight.x);
//                occupancyValues.push_back(bottomRight.y);
//            }
//        }
//    }
//    qDebug() << "Amount of render cubes: " << positions.size();
//    glBindBuffer(GL_TEXTURE_BUFFER, _renderCubePositionsBufferID);
//    glBufferData(GL_TEXTURE_BUFFER, positions.size() * sizeof(mv::Vector3f), positions.data(), GL_DYNAMIC_DRAW);
//
//    glBindBuffer(GL_TEXTURE_BUFFER, _renderCubeOccupancyBufferID);
//    glBufferData(GL_TEXTURE_BUFFER, occupancyValues.size() * sizeof(float), occupancyValues.data(), GL_DYNAMIC_DRAW);
//
//    _renderCubeAmount = positions.size();
//}


void VolumeRenderer::updataDataTexture()
{
    QPair<float, float> scalarDataRange;


    if (_volumeDataset.isValid()) {
        _renderCubesUpdated = false; // We need to update the render cubes as the data has changed

        if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_FULL || _renderMode == RenderMode::MaterialTransition_FULL) {
            int blockAmount = std::ceil(float(_compositeIndices.size()) / 4.0f) * 4; //Since we always assume textures with 4 dimensions all of which need to be filled
            _textureData = std::vector<float>(blockAmount * _volumeDataset->getNumberOfVoxels());
            _volumeTextureSize = _volumeDataset->getVolumeAtlasData(_compositeIndices, _textureData, scalarDataRange);
            _fullDataMemorySize = sizeof(float) * _textureData.size(); // in bytes
            qDebug() << "Full data memory size: " << _fullDataMemorySize;
            // Generate and bind a 3D texture
            _volumeTexture.bind();
            _volumeTexture.setData(_volumeTextureSize.x, _volumeTextureSize.y, _volumeTextureSize.z, _textureData, 4);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            _volumeTexture.release(); // Unbind the texture
        }
        else if (_renderMode == RenderMode::NN_MaterialTransition || _renderMode == RenderMode::Alt_NN_MaterialTransition || _renderMode == RenderMode::Smooth_NN_MaterialTransition) {
            if (!_tfDataset.isValid() || !_reducedPosDataset.isValid()) {
                qCritical() << "No transfer function data set";
                return;
            }
            int pointAmount = _volumeDataset->getNumberOfVoxels();
            _textureData = std::vector<float>(pointAmount * 4);
            //textureData = std::vector<float>(pointAmount * 2);
            _volumeTextureSize = _volumeSize;

            //Get the correct data into textureData 
            std::vector<float> positionData = std::vector<float>(pointAmount * 2);
            _reducedPosDataset->populateDataForDimensions(positionData, std::vector<int>{0, 1});
            normalizePositionData(positionData);
            int width = _materialPositionDataset->getImageSize().width();
            for (int i = 0; i < pointAmount; i++)
            {
                int x = positionData[i * 2];
                int y = positionData[i * 2 + 1];
                int pixelPos = (y * width + x);

                _textureData[i * 4] = _materialPositionImage[pixelPos];
                _textureData[(i * 4) + 1] = _materialPositionImage[pixelPos];
                _textureData[(i * 4) + 2] = _materialPositionImage[pixelPos];
                _textureData[(i * 4) + 3] = _materialPositionImage[pixelPos];
            }
            // Generate and bind a 3D texture
            _volumeTexture.bind();
            _volumeTexture.setData(_volumeTextureSize.x, _volumeTextureSize.y, _volumeTextureSize.z, _textureData, 4);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            _volumeTexture.release(); // Unbind the texture

            if (_renderMode == RenderMode::Smooth_NN_MaterialTransition)
                updateAuxilairySmoothNNTextures();
        }
        else if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_COLOR || _renderMode == RenderMode::NN_MULTIDIMENSIONAL_COMPOSITE) {
            if (!_tfDataset.isValid() || !_reducedPosDataset.isValid()) {
                qCritical() << "No transfer function data set";
                return;
            }
            int pointAmount = _volumeDataset->getNumberOfVoxels();
            _textureData = std::vector<float>(pointAmount * 4);
            //textureData = std::vector<float>(pointAmount * 2);
            _volumeTextureSize = _volumeSize;

            //Get the correct data into textureData 
            std::vector<float> positionData = std::vector<float>(pointAmount * 2);
            _reducedPosDataset->populateDataForDimensions(positionData, std::vector<int>{0, 1});
            normalizePositionData(positionData);
            int width = _tfDataset->getImageSize().width();

            for (int i = 0; i < pointAmount; i++)
            {
                int x = positionData[i * 2];
                int y = (positionData[i * 2 + 1]);
                int pixelPos = (y * width + x) * 4;

                //qDebug() << "pixelPos: " << pixelPos;
                _textureData[i * 4] = _tfImage[pixelPos];
                _textureData[(i * 4) + 1] = _tfImage[pixelPos + 1];
                _textureData[(i * 4) + 2] = _tfImage[pixelPos + 2];
                _textureData[(i * 4) + 3] = _tfImage[pixelPos + 3];
            }
            // Generate and bind a 3D texture
            _volumeTexture.bind();
            _volumeTexture.setData(_volumeTextureSize.x, _volumeTextureSize.y, _volumeTextureSize.z, _textureData, 4);
            _volumeTexture.release(); // Unbind the texture
        }
        else if (_renderMode == RenderMode::MIP) {
            _textureData = std::vector<float>(_volumeDataset->getNumberOfVoxels());
            _volumeTextureSize = _volumeDataset->getVolumeAtlasData(std::vector<uint32_t>{ uint32_t(_mipDimension) }, _textureData, scalarDataRange, 1);

            // Generate and bind a 3D texture
            _volumeTexture.bind();
            _volumeTexture.setData(_volumeTextureSize.x, _volumeTextureSize.y, _volumeTextureSize.z, _textureData, 1);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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
        if (mode == RenderMode::MaterialTransition_2D || mode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_2D_POS)
            return 2;
        if (mode == RenderMode::NN_MaterialTransition || mode == RenderMode::Alt_NN_MaterialTransition || mode == RenderMode::Smooth_NN_MaterialTransition)
            return 3;
        if (mode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_COLOR || mode == RenderMode::NN_MULTIDIMENSIONAL_COMPOSITE)
            return 4;
        if (mode == RenderMode::MIP)
            return 5;
        return 0; // Unknown group
        };

    int currentGroup = getRenderModeGroup(givenMode);

    // Only set _dataSettingsChanged to true if the group changes
    if (getRenderModeGroup(_renderMode) != currentGroup)
        _dataSettingsChanged = true;

    // The NN and Linear version of the same render mode share the same volume texture but they do require slightly different settings
    if (currentGroup == 4) {
        _volumeTexture.bind();
        if (_renderMode == RenderMode::NN_MULTIDIMENSIONAL_COMPOSITE) {
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
        _fullDataModeBatch = -1; // We don't need to use the full data in these modes
    }

    _renderMode = givenMode;
}

void VolumeRenderer::setMIPDimension(int mipDimension)
{
    if (_mipDimension != mipDimension)
        _dataSettingsChanged = true;
    _mipDimension = mipDimension;
}

void VolumeRenderer::setUseShading(bool useShading)
{
    _useShading = useShading;
}

void VolumeRenderer::setUseEmptySpaceSkipping(bool useEmptySpaceSkipping)
{
    _useEmptySpaceSkipping = useEmptySpaceSkipping;
}

void VolumeRenderer::setRenderCubeSize(float renderCubeSize)
{
    if (_renderCubeSize != renderCubeSize) {
        _renderCubeSize = renderCubeSize;
        //updateRenderCubes();
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

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_BUFFER, _renderCubeOccupancyTexID);
    shader.uniform1i("renderCubeOccupancy", 6);

    _tfRectangleDataTexture.bind(7);
    shader.uniform1i("tfRectangleData", 7);


    shader.uniformMatrix4f("u_modelViewProjection", _mvpMatrix.constData());
    shader.uniformMatrix4f("u_model", _modelMatrix.constData());
    shader.uniform3fv("u_minClippingPlane", 1, &_minClippingPlane);
    shader.uniform3fv("u_maxClippingPlane", 1, &_maxClippingPlane);

    mv::Vector3f renderCubeSize = _volumeSize;
    if (_useEmptySpaceSkipping)
        renderCubeSize = mv::Vector3f(_renderCubeSize / _volumeSize.x, _renderCubeSize / _volumeSize.y, _renderCubeSize / _volumeSize.z);
    shader.uniform3fv("renderCubeSize", 1, &renderCubeSize);

    shader.uniform1i("useEmptySpaceSkipping", _useEmptySpaceSkipping);
    shader.uniform1i("renderType", _renderMode);

    // The actual rendering step
    _vao.bind();
    if (_useEmptySpaceSkipping)
        glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, _renderCubeAmount);
    else
        glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, 1);
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


void VolumeRenderer::prepareHNSW()
{
    if (!_volumeDataset.isValid()) {
        qCritical() << "Volume dataset is not valid. Cannot prepare HNSW.";
        return;
    }

    // Get the number of voxels and dimensions from the dataset
    uint32_t numVoxels = _volumeDataset->getNumberOfVoxels();
    uint32_t dimensions = _volumeDataset->getComponentsPerVoxel();

    qDebug() << "Preparing HNSW with" << numVoxels << "voxels and" << dimensions << "dimensions.";

    // Initialize HNSW space and index
    _hnswSpace = std::make_unique<hnswlib::L2Space>(dimensions); //If we use a local parameter here instead of a member variable we get a crash later on in the program when calling the hwnsIndex again
    _hnswIndex = std::make_unique<hnswlib::HierarchicalNSW<float>>(
        _hnswSpace.get(),
        numVoxels,
        _hnswM,
        _hnswEfConstruction
    );

    // Populate HNSW index with volume data.
    std::vector<float> voxelData(dimensions * numVoxels);
    QPair<float, float> scalarDataRange;
    _volumeDataset->getVolumeData(_compositeIndices, voxelData, scalarDataRange);

    // Generate random data in the range [0, 1]
    //std::random_device rd;
    //std::mt19937 gen(rd());
    //std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    //// Parallelize data generation using OpenMP.
    //#pragma omp parallel for schedule(static)
    //for (int32_t i = 0; i < numVoxels; i++) {
    //    for (int32_t j = 0; j < dimensions; j++) {
    //        // Each element is a random float in [0, 1]
    //        voxelData[i * dimensions + j] = dis(gen);
    //    }
    //}

    // Optionally, print the first few voxel vectors for inspection.
    //for (uint32_t i = 10000; i < std::min(numVoxels, (uint32_t)100000); i++) {
    //    QString s = "Voxel " + QString::number(i) + ":";
    //    for (uint32_t j = 0; j < dimensions; j++) {
    //        s += " " + QString::number(voxelData[i * dimensions + j]);
    //    }
    //    qDebug() << s;
    //}

    //for (uint32_t i = 0; i < numVoxels; ++i) {
    //    // Normalize the voxel data to the range [0, 1]
    //    for (uint32_t j = 0; j < dimensions; ++j) {
    //        voxelData[i * dimensions + j] = (voxelData[i * dimensions + j] - scalarDataRange.first) / (scalarDataRange.second - scalarDataRange.first);
    //    }
    //}

    // Add points to the HNSW index
    for (uint32_t i = 0; i < numVoxels; ++i) {
        _hnswIndex->addPoint(voxelData.data() + i * dimensions, i);
    }

    // Set a high ef for query-time (improves recall, at the expense of query latency)
    //_hnswIndex->setEf(_hwnsEfSearch);

    // Test recall: query each point for its nearest neighbor.
    float correct = 0;
#pragma omp parallel for schedule(guided)
    for (int32_t i = 0; i < numVoxels; i++) {
        const float* queryPoint = voxelData.data() + i * dimensions;
        std::priority_queue<std::pair<float, hnswlib::labeltype>> result = _hnswIndex->searchKnn(queryPoint, 1);
        float distance = result.top().first;
#pragma omp critical
        {
            if (distance == 0)
                correct++;
        }
    }
    float recall = correct / numVoxels;
    std::cout << "Recall: " << recall << "\n";
}



// Method that handles large query batches using hsnswlib faster then calling searchKnn for each query in a for loop by making use of parallelization 
std::vector<std::vector<std::pair<float, hnswlib::labeltype>>> VolumeRenderer::batchSearch(
    const std::vector<float>& queryData, // Flat vector: each query is (dimensions) floats
    uint32_t dimensions,                      // Dimensionality of a single query
    int k                                // Number of nearest neighbors to retrieve
) {
    if (queryData.size() % dimensions != 0) {
        qCritical() << "Query data size is not a multiple of dimensions.";
    }

    // Check that the index is valid.
    if (!_hnswIndex) {
        qCritical() << "HNSW index is not initialized.";
        return {};
    }

    int64_t numQueries = static_cast<int64_t>(queryData.size() / dimensions);

    // Prepare a container to hold result vectors (one per query) with k elements each containing a pair of distance and label.
    std::vector<std::vector<std::pair<float, hnswlib::labeltype>>> batchResults(numQueries);
    qDebug() << "Batch search size: " << batchResults.size();

    //float* startQueryDataPtr = const_cast<float*>(queryData.data());

#pragma omp parallel for schedule(guided)
    for (int64_t i = 0; i < numQueries; i++) { // it is important to use int64_t here to avoid overflow crashes
        // Find pointer to the start of the i-th query.
        const float* query = queryData.data() + static_cast<int64_t>(i * dimensions);
        std::priority_queue<std::pair<float, hnswlib::labeltype>> resultQueue = _hnswIndex->searchKnn(query, k);

        // Convert the priority queue to a vector.
        std::vector<std::pair<float, hnswlib::labeltype>> answers;
        while (!resultQueue.empty()) {
            answers.push_back(resultQueue.top());
            resultQueue.pop();
        }

        batchResults[i] = answers;
    }

    qDebug() << "Batch search size: " << batchResults.size() << "x" << batchResults[0].size();
    return batchResults;
}

// Extracts the frontfaces and backfaces texture data into a vector of floats
void VolumeRenderer::getFacesTextureData(std::vector<float>& frontfacesData, std::vector<float>& backfacesData)
{
    _frontfacesTexture.bind();
    _backfacesTexture.bind();

    // Read the frontfaces texture data (we request RGB – assuming alpha is not needed)
    glBindTexture(GL_TEXTURE_2D, _frontfacesTexture.getHandle());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, frontfacesData.data());

    // Read the backfaces texture data
    glBindTexture(GL_TEXTURE_2D, _backfacesTexture.getHandle());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, backfacesData.data());

    // Unbind the textures
    glBindTexture(GL_TEXTURE_2D, 0);
}

// This function is used to get the GPU data in full data mode.
// It uses the frontfaces and backfaces texture data to compute the ray lengths and sample counts for each pixel.
// It then uses this data to create batches of pixels that can be processed in the GPU.
void VolumeRenderer::getGPUFullDataModeBatches(std::vector<float>& frontfacesData, std::vector<float>& backfacesData, std::vector<size_t>& _subsetsMemory, std::vector<std::vector<int>>& _GPUBatches, std::vector<std::vector<int>>& _GPUBatchesStartIndex)
{

    _GPUBatches.clear();
    _GPUBatchesStartIndex.clear();
    _subsetsMemory.clear();

    // Get the dimensions of the textures
    int width = _screenSize.width();
    int height = _screenSize.height();

    // Check if the frontfaces and backfaces data are valid
    if (frontfacesData.size() != backfacesData.size() || frontfacesData.size() != width * height * 3)
    {
        qCritical() << "Frontfaces and backfaces data size mismatch or invalid size.";
        return;
    }

    //Create small batches of pixels that are spread out over the whole image ---

    int numBatches = 64; // Number of batches to divide the data into. (Tune as needed.)
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
            // Skip pixel if it does not hit the volume.
            if (frontfacesData[idx * 3] == 0.0f)
                continue;

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
            int sampleCount = static_cast<int>(rayLength / _stepSize);

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
    size_t availableMemoryInBytes = _fullGPUMemorySize - _fullDataMemorySize - 100000; // ~100MB reserved for other data
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
void VolumeRenderer::retrieveBatchFullData(std::vector<float>& cpuOutput, std::vector<size_t> _subsetsMemory, int batchIndex, std::vector<std::vector<int>> _GPUBatches, std::vector<std::vector<int>> _GPUBatchesStartIndex, bool deleteBuffers)
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
    _fullDataSamplerComputeShader->setUniformValue("invFaceTexSize", QVector2D(1.0f / _screenSize.width(), 1.0f / _screenSize.height()));

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
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, _subsetsMemory[batchIndex], cpuOutput.data());

    // Unbind the output SSBO.
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
QVector2D computeMean(const std::vector<QVector2D>& points) {
    if (points.empty())
        return QVector2D(0, 0);

    QVector2D sum = std::accumulate(points.begin(), points.end(), QVector2D(0, 0));
    return sum / static_cast<float>(points.size());
}

// TODO : This function should be moved to a more appropriate location, as it is not specific to the VolumeRenderer class.
// Compute the weighted mean of a std::vector<QVector2D> given corresponding weight values.
QVector2D computeWeightedMean(const std::vector<QVector2D>& points, const std::vector<float>& weights) {
    if (points.empty() || points.size() != weights.size())
        return QVector2D(0, 0);

    QVector2D weightedSum(0, 0);
    float totalWeight = 0.0f;
    for (size_t i = 0; i < points.size(); ++i) {
        weightedSum += points[i] * weights[i];
        totalWeight += weights[i];
    }
    return (totalWeight > 0.0f) ? (weightedSum / totalWeight) : QVector2D(0, 0);
}

// This function renders the full data to the screen using the composite shader.
// It takes the GPU batches and their start indices, as well as the mean positions of the samples.
// The function also takes and updates the composite texture of the previous results as input, such that all previous batches are also rendered to the screen.
void VolumeRenderer::renderBatchToScreen(std::vector<std::vector<int>>& _GPUBatchesStartIndex, int batchIndex, uint32_t sampleDim, std::vector<float>& meanPositions, std::vector<std::vector<int>>& _GPUBatches)
{
    int width = _screenSize.width();
    int height = _screenSize.height();

    std::vector<int> mappingSampleStart(_GPUBatchesStartIndex[batchIndex].size() + 1); // Start index for each ray as if each sample takes one space (we multiply by 2 in the shader)
    for (size_t i = 0; i < mappingSampleStart.size(); i++) {
        mappingSampleStart[i] = (_GPUBatchesStartIndex[batchIndex][i]); // The GPU start index has all components for each sample, so we need to divide by the number of components to get the samplePos
    }
    mappingSampleStart[mappingSampleStart.size() - 1] = meanPositions.size() / 2; //Since the mappingSampleStart array keeps the indices for the sample amount and the meanPosition vector contains two floats per sample
    int numRays = _GPUBatchesStartIndex[batchIndex].size();

    std::vector<int> rayIDTextureData(_screenSize.width() * _screenSize.height(), -1);
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, _screenSize.width(), _screenSize.height(), 0, GL_RED_INTEGER, GL_INT, rayIDTextureData.data());
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

    // Swap over to a differnt framebuffer that we can use to write the results to a texture instead of the screen.
    _framebuffer.bind();
    _framebuffer.setTexture(GL_COLOR_ATTACHMENT0, _prevFullCompositeTexture);
    //_framebuffer.setTexture(GL_DEPTH_ATTACHMENT, _depthTexture);

    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    // --- Set up and bind the composite shader ---
    _fullDataCompositeShader.bind();

    // Bind our textures to the expected units.
    rayIDTexture.bind(0);
    _fullDataCompositeShader.uniform1i("rayIDTexture", 0);
    _tfTexture.bind(1);
    _fullDataCompositeShader.uniform1i("tfTexture", 1);

    _fullDataCompositeShader.uniform2f("invFaceTexSize", 1.0f / float(width), 1.0f / float(height));
    _fullDataCompositeShader.uniform2f("invTfTexSize", 1.0f / _tfDataset->getImageSize().width(), 1.0f / _tfDataset->getImageSize().height());

    // Render a full-screen quad to composite the current batch's results over prevFullCompositeTexture.
    drawDVRQuad(_fullDataCompositeShader);
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

void VolumeRenderer::ComputeMeanOfNN(std::vector<std::vector<std::pair<float, hnswlib::labeltype>>>& nnResults, int k, std::vector<float>& positionData, bool useWeightedMean, std::vector<float>& meanPositions)
{
    float epsilon = 1.0f;  // To avoid division by zero.
#pragma omp parallel for schedule(guided)
    for (int32_t i = 0; i < static_cast<int32_t>(nnResults.size()); i++) {
        std::vector<float> weights(k, 0.0f);
        std::vector<QVector2D> candidatePositions(k, QVector2D(0.0f, 0.0f));
        int j = 0;
        const auto& neighbors = nnResults[i];
        for (const auto& entry : neighbors) {
            int32_t index = static_cast<int32_t>(entry.second);
            weights[j] = (1.0f / (entry.first + epsilon));  // Inverse distance is used as weight
            float posX = positionData[index * 2];
            float posY = positionData[index * 2 + 1];
            candidatePositions[j] = QVector2D(posX, posY);
            j++;
        }
        QVector2D meanPos;
        if (useWeightedMean)
            meanPos = computeWeightedMean(candidatePositions, weights);
        else
            meanPos = computeMean(candidatePositions);
        meanPositions[i * 2] = meanPos.x();
        meanPositions[i * 2 + 1] = meanPos.y();
    }
}

void VolumeRenderer::updateRenderModeParameters()
{
    // 3. Get the screen dimensions and allocate arrays to read the front and back face textures.
    int screenWidth = _screenSize.width();
    int screenHeight = _screenSize.height();

    std::vector<float> frontfacesData(screenWidth * screenHeight * 3);
    std::vector<float> backfacesData(screenWidth * screenHeight * 3);
    getFacesTextureData(frontfacesData, backfacesData);
    qDebug() << "Front and backfaces data retrieved.";

    // Create the GPU full data batches. ---
    getGPUFullDataModeBatches(frontfacesData, backfacesData, _subsetsMemory, _GPUBatches, _GPUBatchesStartIndex);
    qDebug() << "GPU full data mode batches created.";

    // Initialize the previous composite texture, this texture will hold the cumulative composite result. ---
    _prevFullCompositeTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, screenWidth, screenHeight, 0, GL_RGB, GL_FLOAT, nullptr);
    _prevFullCompositeTexture.release();
    qDebug() << "Previous composite texture initialized.";
}


void VolumeRenderer::renderCompositeFull()
{
    // Check available GPU memory for the batch transfer. ---
    size_t availableMemoryInBytes = _fullGPUMemorySize - _fullDataMemorySize - 100000; // Reserve ~100MB for other data.
    if (availableMemoryInBytes < 0) {
        qCritical() << "Not enough GPU memory available for the GPU-CPU batch transfer.";
        return;
    }

    // Make sure the ANN (e.g. hnswlib) is prepared for the dataset. ---
    if (!_ANNAlgorithmTrained) {
        prepareHNSW();
        _ANNAlgorithmTrained = true;
    }

    // Initialize the GPU full data mode parameters if not already done. ---
    if (_fullDataModeBatch == -1) {
        qDebug() << "Available GPU memory for batch transfer:" << availableMemoryInBytes / (1024 * 1024) << "MB";
        qDebug() << "Rendering composite full data...";

        updateRenderModeParameters();
        _fullDataModeBatch = 0;
    }

    // Process a batch (given by the batchIndex) ---
    // As each batch is processed, its rendered results are composited over the previous result.
    qDebug() << "Processing batch" << _fullDataModeBatch << "of" << _GPUBatches.size();

    // Retrieve the full data for the batch from the GPU Compute Shader. ---
    std::vector<float> cpuOutput;
    retrieveBatchFullData(cpuOutput, _subsetsMemory, _fullDataModeBatch, _GPUBatches, _GPUBatchesStartIndex, true);
    qDebug() << "Batch full data retrieved from GPU compute shader for batch" << _fullDataModeBatch;

    // Run approximate nearest-neighbour search on the retrieved CPU data. ---
    uint32_t sampleDim = _volumeDataset->getComponentsPerVoxel();
    int k = 3;
    std::vector<std::vector<std::pair<float, hnswlib::labeltype>>> nnResults = batchSearch(cpuOutput, sampleDim, k);
    cpuOutput.clear();  // Free memory immediately.
    qDebug() << "Approximate nearest neighbour search completed for batch" << _fullDataModeBatch;

    // Retrieve the reduced 2D position data (e.g. from a dimension reduction dataset). ---
    int pointAmount = _volumeDataset->getNumberOfVoxels() * 2; // two floats per voxel.
    std::vector<float> positionData(pointAmount);
    _reducedPosDataset->populateDataForDimensions(positionData, std::vector<int>{0, 1});
    normalizePositionData(positionData);

    // Compute the mean (2D) positions for each GPU sample based on its nearest neighbours. ---
    std::vector<float> meanPositions(nnResults.size() * 2);
    bool useWeightedMean = false;  // change to "true" if you need weighting.
    ComputeMeanOfNN(nnResults, k, positionData, useWeightedMean, meanPositions);
    nnResults.clear();
    qDebug() << "Mean positions computed for batch" << _fullDataModeBatch;

    // Composite this batch’s result over the previous composite and update the texture. ---
    renderBatchToScreen(_GPUBatchesStartIndex, _fullDataModeBatch, sampleDim, meanPositions, _GPUBatches);
    qDebug() << "Rendered batch" << _fullDataModeBatch << "to composite texture.";

    if (_fullDataModeBatch == _GPUBatches.size() - 1) {
        _fullDataModeBatch = -1;
        qDebug() << "Composite full rendering completed.";
    }
    else {
        _fullDataModeBatch++;
    }
}



void VolumeRenderer::renderComposite2DPos()
{
    setDefaultRenderSettings();

    ////Set textures and uniforms
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
    _2DCompositeShader.uniform2f("invFaceTexSize", 1.0f / _screenSize.width(), 1.0f / _screenSize.height());
    _2DCompositeShader.uniform2f("invTfTexSize", 1.0f / _tfDataset->getImageSize().width(), 1.0f / _tfDataset->getImageSize().height());


    drawDVRQuad(_2DCompositeShader);

    // Restore depth clear value
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);
}

// Render the volume colors directly
void VolumeRenderer::renderCompositeColor()
{
    setDefaultRenderSettings();

    ////Set textures and uniforms
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
    if (_useCustomRenderSpace) {
        volumeSize = _renderSpace;
        invVolumeSize = mv::Vector3f(1.0f / _renderSpace.x, 1.0f / _renderSpace.y, 1.0f / _renderSpace.z);
    }
    else {
        volumeSize = _volumeSize;
        invVolumeSize = mv::Vector3f(1.0f / _volumeSize.x, 1.0f / _volumeSize.y, 1.0f / _volumeSize.z);
    }

    _colorCompositeShader.uniform3fv("dimensions", 1, &volumeSize);
    _colorCompositeShader.uniform3fv("invDimensions", 1, &invVolumeSize);
    _colorCompositeShader.uniform2f("invFaceTexSize", 1.0f / _screenSize.width(), 1.0f / _screenSize.height());
    _colorCompositeShader.uniform2f("invTfTexSize", 1.0f / _tfDataset->getImageSize().width(), 1.0f / _tfDataset->getImageSize().height());


    drawDVRQuad(_colorCompositeShader);

    // Restore depth clear value
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);
}

// Render using a standard MIP algorithm on a 1D slice of the volume
void VolumeRenderer::render1DMip()
{
    setDefaultRenderSettings();

    ////Set textures and uniforms
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
    _1DMipShader.uniform2f("invFaceTexSize", 1.0f / _screenSize.width(), 1.0f / _screenSize.height());


    drawDVRQuad(_1DMipShader);

    // Restore depth clear value
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);
}

void VolumeRenderer::renderMaterialTransitionFull()
{
    //TODO
}

void VolumeRenderer::renderMaterialTransition2D()
{
    setDefaultRenderSettings();

    ////Set textures and uniforms
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
    _materialTransition2DShader.uniform2f("invFaceTexSize", 1.0f / _screenSize.width(), 1.0f / _screenSize.height());
    _materialTransition2DShader.uniform2f("invTfTexSize", 1.0f / _materialPositionDataset->getImageSize().width(), 1.0f / _materialPositionDataset->getImageSize().height());
    _materialTransition2DShader.uniform2f("invMatTexSize", 1.0f / _materialTransitionDataset->getImageSize().width(), 1.0f / _materialTransitionDataset->getImageSize().height());

    drawDVRQuad(_materialTransition2DShader);

    // Restore depth clear value
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);
}

void VolumeRenderer::renderNNMaterialTransition()
{
    setDefaultRenderSettings();
    ////Set textures and uniforms
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
    _nnMaterialTransitionShader.uniform2f("invFaceTexSize", 1.0f / _screenSize.width(), 1.0f / _screenSize.height());
    _nnMaterialTransitionShader.uniform2f("invMatTexSize", 1.0f / _materialTransitionDataset->getImageSize().width(), 1.0f / _materialTransitionDataset->getImageSize().height());

    drawDVRQuad(_nnMaterialTransitionShader);
    // Restore depth clear value
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);
}

void VolumeRenderer::renderAltNNMaterialTransition()
{
    setDefaultRenderSettings();
    ////Set textures and uniforms
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
    _altNNMaterialTransitionShader.uniform2f("invFaceTexSize", 1.0f / _screenSize.width(), 1.0f / _screenSize.height());
    _altNNMaterialTransitionShader.uniform2f("invMatTexSize", 1.0f / _materialTransitionDataset->getImageSize().width(), 1.0f / _materialTransitionDataset->getImageSize().height());

    drawDVRQuad(_altNNMaterialTransitionShader);
    // Restore depth clear value
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);
}

//void VolumeRenderer::renderSmoothNNMaterialTransition()
//{
//    setDefaultRenderSettings();
//    ////Set textures and uniforms
//    _smoothNNMaterialTransitionShader.bind();
//
//    _backfacesTexture.bind(0);
//    _smoothNNMaterialTransitionShader.uniform1i("directions", 0);
//
//    _volumeTexture.bind(1);
//    _smoothNNMaterialTransitionShader.uniform1i("volumeData", 1);
//
//    _materialTransitionTexture.bind(2);
//    _smoothNNMaterialTransitionShader.uniform1i("materialTexture", 2);
//
//    //_materialPositionTexture.bind(3);
//    //_smoothNNMaterialTransitionShader.uniform1i("NeighbourIndicesTexture", 3);
//
//    _smoothNNMaterialTransitionShader.uniform1f("stepSize", _stepSize);
//    _smoothNNMaterialTransitionShader.uniform1i("useShading", _useShading);
//    _smoothNNMaterialTransitionShader.uniform3fv("camPos", 1, &_cameraPos);
//    _smoothNNMaterialTransitionShader.uniform3fv("lightPos", 1, &_cameraPos);
//    if (_useCustomRenderSpace)
//        _smoothNNMaterialTransitionShader.uniform3fv("dimensions", 1, &_renderSpace);
//    else
//        _smoothNNMaterialTransitionShader.uniform3fv("dimensions", 1, &_volumeSize);
//    drawDVRQuad(_smoothNNMaterialTransitionShader);
//    // Restore depth clear value
//    glClear(GL_DEPTH_BUFFER_BIT);
//    glDepthFunc(GL_LEQUAL);
//}

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
        //if (!_renderCubesUpdated)
        //    updateRenderCubes();
        if (_renderMode == RenderMode::MaterialTransition_FULL)
            renderMaterialTransitionFull();
        else if (_renderMode == RenderMode::MaterialTransition_2D)
            renderMaterialTransition2D();
        else if (_renderMode == RenderMode::NN_MaterialTransition)
            renderNNMaterialTransition();
        else if (_renderMode == RenderMode::Alt_NN_MaterialTransition)
            renderAltNNMaterialTransition();
        //else if (_renderMode == RenderMode::Smooth_NN_MaterialTransition)
        //    renderSmoothNNMaterialTransition();
        else if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_FULL)
            renderCompositeFull();
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

