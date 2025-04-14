#include "VolumeRenderer.h"
#include <QImage>
#include <random>
#include <QOpenGLWidget>

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

    _renderTexture.create();
    _renderTexture.bind();
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
    //loaded &= _smoothNNMaterialTransitionShader.loadShaderFromFile(":shaders/Quad.vert", ":shaders/SmoothNNMaterialTransition.frag");
    loaded &= _framebufferShader.loadShaderFromFile(":shaders/Quad.vert", ":shaders/Texture.frag");

    if (!loaded) {
        qCritical() << "Failed to load one of the Volume Renderer shaders";
    }
    else {
        qDebug() << "Volume Renderer shaders loaded";
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, renderSize.width(), renderSize.height(), 0, GL_RGBA, GL_FLOAT, nullptr);

    _frontfacesTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, renderSize.width(), renderSize.height(), 0, GL_RGBA, GL_FLOAT, nullptr);

    _depthTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, renderSize.width(), renderSize.height(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    _renderTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, renderSize.width(), renderSize.height(), 0, GL_RGBA, GL_FLOAT, nullptr);

    _screenSize = renderSize;

    glViewport(0, 0, renderSize.width(), renderSize.height());

}

void VolumeRenderer::setData(const mv::Dataset<Volumes>& dataset)
{
    _volumeDataset = dataset;
    _volumeSize = dataset->getVolumeSize().toVector3f();
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

    if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_COLOR || _renderMode == RenderMode::NN_MULTIDIMENSIONAL_COMPOSITE || _renderMode == RenderMode::NN_MaterialTransition || _renderMode == RenderMode::Alt_NN_MaterialTransition || _renderMode == RenderMode::Smooth_NN_MaterialTransition)
        updataDataTexture();
}

void VolumeRenderer::setReducedPosData(const mv::Dataset<Points>& reducedPosData)
{
    _reducedPosDataset = reducedPosData;
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

    int size = _tfDataset->getImageSize().width(); // We use a square texture
    if (_renderMode == RenderMode::MaterialTransition_2D || _renderMode == RenderMode::NN_MaterialTransition || _renderMode == RenderMode::Alt_NN_MaterialTransition || _renderMode == RenderMode::Smooth_NN_MaterialTransition)
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
    mv::Vector3f textureSize;

    if (_volumeDataset.isValid()) {
        _renderCubesUpdated = false; // We need to update the render cubes as the data has changed

        if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_FULL || _renderMode == RenderMode::MaterialTransition_FULL) {
            int blockAmount = std::ceil(float(_compositeIndices.size()) / 4.0f) * 4; //Since we always assume textures with 4 dimensions all of which need to be filled
            _textureData = std::vector<float>(blockAmount * _volumeDataset->getNumberOfVoxels());
            textureSize = _volumeDataset->getVolumeAtlasData(_compositeIndices, _textureData, scalarDataRange);

            // Generate and bind a 3D texture
            _volumeTexture.bind();
            _volumeTexture.setData(textureSize.x, textureSize.y, textureSize.z, _textureData, 4);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            _volumeTexture.release(); // Unbind the texture
        }
        else if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_2D_POS || _renderMode == RenderMode::MaterialTransition_2D) {
            if (!_tfDataset.isValid() || !_reducedPosDataset.isValid()) { // _tfTexture is used in the normilize function
                qCritical() << "No position data set";
                return;
            }
            _textureData = std::vector<float>(_volumeDataset->getNumberOfVoxels() * 2);
            textureSize = _volumeSize;

            _reducedPosDataset->populateDataForDimensions(_textureData, std::vector<int>{0, 1});
            normalizePositionData(_textureData);

            // Generate and bind a 3D texture
            _volumeTexture.bind();
            _volumeTexture.setData(textureSize.x, textureSize.y, textureSize.z, _textureData, 2);
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
            textureSize = _volumeSize;

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
            _volumeTexture.setData(textureSize.x, textureSize.y, textureSize.z, _textureData, 4);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            _volumeTexture.release(); // Unbind the texture

            if (_renderMode == RenderMode::Smooth_NN_MaterialTransition)
                updateAuxilairySmoothNNTextures();
        }
        else if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_COLOR || _renderMode == RenderMode::NN_MULTIDIMENSIONAL_COMPOSITE) {
            if (!_tfDataset.isValid() || !_reducedPosDataset.isValid()){
                qCritical() << "No transfer function data set";
                return;
            }
            int pointAmount = _volumeDataset->getNumberOfVoxels();
            _textureData = std::vector<float>(pointAmount * 4);
            //textureData = std::vector<float>(pointAmount * 2);
            textureSize = _volumeSize;
            
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
            _volumeTexture.setData(textureSize.x, textureSize.y, textureSize.z, _textureData, 4);
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
        else if (_renderMode == RenderMode::MIP) {
            _textureData = std::vector<float>(_volumeDataset->getNumberOfVoxels());
            textureSize = _volumeDataset->getVolumeAtlasData(std::vector<uint32_t>{ uint32_t(_mipDimension) }, _textureData, scalarDataRange, 1);

            // Generate and bind a 3D texture
            _volumeTexture.bind();
            _volumeTexture.setData(textureSize.x, textureSize.y, textureSize.z, _textureData, 1);
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

    if(_renderMode != givenMode)
        _dataSettingsChanged = true;
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
    if(_useCustomRenderSpace)
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

    // We count missed rays as very close to the camera since we want to select the furtest away geometry in this renderpass
    glClearDepth(0.0f); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_GREATER);

    _surfaceShader.bind();
    drawDVRRender(_surfaceShader);

    // Restore depth clear value
    glClearDepth(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);

    _framebuffer.release();
}

void VolumeRenderer::renderCompositeFull()
{
    //TODO
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
    if (_useCustomRenderSpace)
        volumeSize = mv::Vector3f(1.0f / _renderSpace.x, 1.0f / _renderSpace.y, 1.0f / _renderSpace.z);
    else
        volumeSize = mv::Vector3f(1.0f / _volumeSize.x, 1.0f / _volumeSize.y, 1.0f / _volumeSize.z);

    _2DCompositeShader.uniform3fv("invDimensions", 1, &volumeSize);
    _2DCompositeShader.uniform2f("invDirTexSize", 1.0f / _screenSize.width(), 1.0f / _screenSize.height());
    _2DCompositeShader.uniform2f("invTfTexSize", 1.0f / _materialPositionDataset->getImageSize().width(), 1.0f / _materialPositionDataset->getImageSize().height());


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
    if (_useCustomRenderSpace)
        volumeSize = mv::Vector3f(1.0f / _renderSpace.x, 1.0f / _renderSpace.y, 1.0f / _renderSpace.z);
    else
        volumeSize = mv::Vector3f(1.0f / _volumeSize.x, 1.0f / _volumeSize.y, 1.0f / _volumeSize.z);

    _colorCompositeShader.uniform3fv("invDimensions", 1, &volumeSize);
    _colorCompositeShader.uniform2f("invDirTexSize", 1.0f / _screenSize.width(), 1.0f / _screenSize.height());
    _colorCompositeShader.uniform2f("invTfTexSize", 1.0f / _materialPositionDataset->getImageSize().width(), 1.0f / _materialPositionDataset->getImageSize().height());


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
    if (_useCustomRenderSpace)
        volumeSize = mv::Vector3f(1.0f / _renderSpace.x, 1.0f / _renderSpace.y, 1.0f / _renderSpace.z);
    else
        volumeSize = mv::Vector3f(1.0f / _volumeSize.x, 1.0f / _volumeSize.y, 1.0f / _volumeSize.z);

    _1DMipShader.uniform3fv("invDimensions", 1, &volumeSize);
    _1DMipShader.uniform2f("invDirTexSize", 1.0f / _screenSize.width(), 1.0f / _screenSize.height());
    _1DMipShader.uniform2f("invMatTexSize", 1.0f / _materialTransitionDataset->getImageSize().width(), 1.0f / _materialTransitionDataset->getImageSize().height());


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
    if (_useCustomRenderSpace)
        volumeSize = mv::Vector3f(1.0f / _renderSpace.x, 1.0f / _renderSpace.y, 1.0f / _renderSpace.z);
    else
        volumeSize = mv::Vector3f(1.0f / _volumeSize.x, 1.0f / _volumeSize.y, 1.0f / _volumeSize.z);

    _materialTransition2DShader.uniform3fv("invDimensions", 1, &volumeSize);
    _materialTransition2DShader.uniform2f("invDirTexSize", 1.0f / _screenSize.width(), 1.0f / _screenSize.height());
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
    if (_useCustomRenderSpace)
        volumeSize = mv::Vector3f(1.0f / _renderSpace.x, 1.0f / _renderSpace.y, 1.0f / _renderSpace.z);
    else
        volumeSize = mv::Vector3f(1.0f / _volumeSize.x, 1.0f / _volumeSize.y, 1.0f / _volumeSize.z);

    _nnMaterialTransitionShader.uniform3fv("invDimensions", 1, &volumeSize);
    _nnMaterialTransitionShader.uniform2f("invDirTexSize", 1.0f / _screenSize.width(), 1.0f / _screenSize.height());
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
    if (_useCustomRenderSpace)
        volumeSize = mv::Vector3f(1.0f / _renderSpace.x, 1.0f / _renderSpace.y, 1.0f / _renderSpace.z);
    else
        volumeSize = mv::Vector3f(1.0f / _volumeSize.x, 1.0f / _volumeSize.y, 1.0f / _volumeSize.z);

    _altNNMaterialTransitionShader.uniform3fv("invDimensions", 1, &volumeSize);
    _altNNMaterialTransitionShader.uniform2f("invDirTexSize", 1.0f / _screenSize.width(), 1.0f / _screenSize.height());
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
    _surfaceShader.bind();

    updateMatrices();
    renderDirections();

    glBindFramebuffer(GL_FRAMEBUFFER, _defaultFramebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
        renderDirectionsTexture();
    }
}

void VolumeRenderer::renderDirectionsTexture()
{
    _frontfacesTexture.bind(0);
    _framebufferShader.uniform1i("tex", 0);
    drawDVRRender(_framebufferShader);
}

void VolumeRenderer::destroy()
{
    _vao.destroy();
    _vboCube.destroy();
    _iboCube.destroy();
    _surfaceShader.destroy();
    _framebufferShader.destroy();
}

