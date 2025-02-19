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

    // initialize textures and bind them to the framebuffer
    _frontfacesTexture.create();
    _frontfacesTexture.bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    _directionsTexture.create();
    _directionsTexture.bind();
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
    loaded &= _directionsShader.loadShaderFromFile(":shaders/Surface.vert", ":shaders/Directions.frag"); 
    loaded &= _2DCompositeShader.loadShaderFromFile(":shaders/Surface.vert", ":shaders/2DComposite.frag");
    loaded &= _colorCompositeShader.loadShaderFromFile(":shaders/Surface.vert", ":shaders/ColorComposite.frag");
    loaded &= _1DMipShader.loadShaderFromFile(":shaders/Surface.vert", ":shaders/1DMip.frag");
    loaded &= _materialTransition2DShader.loadShaderFromFile(":shaders/Surface.vert", ":shaders/MaterialTransition2D.frag");
    loaded &= _framebufferShader.loadShaderFromFile(":shaders/Quad.vert", ":shaders/Texture.frag");

    if (!loaded) {
        qCritical() << "Failed to load one of the Volume Renderer shaders";
    }
    else {
        qDebug() << "Volume Renderer shaders loaded";
    }

    // Initialize a cube mesh 
    const std::array<float, 24> vertices{
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 1.0f,
    };

    const std::array<unsigned, 36> indices{
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

    // Cube arrays
    _vao.create();
    _vao.bind();

    _vbo.create();
    _vbo.bind();
    _vbo.allocate(vertices.data(), vertices.size() * sizeof(float));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    _ibo.create();
    _ibo.bind();
    _ibo.allocate(indices.data(), indices.size() * sizeof(unsigned));

    _vao.release();
    _vbo.release();
    _ibo.release();
    qDebug() << "VolumeRenderer initialized";
}


void VolumeRenderer::resize(QSize renderSize)
{
    _directionsTexture.bind();
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
    _imageData = QVector<float>(textureDims.width() * textureDims.height() * 4);
    QPair<float, float> scalarDataRange;
    _tfDataset->getImageScalarData(0, _imageData, scalarDataRange);

    _scalarImageDataRange = scalarDataRange;

    _tfTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, textureDims.width(), textureDims.height(), 0, GL_RGBA, GL_FLOAT, _imageData.data());
    _tfTexture.release();

    if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_COLOR)
        updataDataTexture();
}

void VolumeRenderer::setReducedPosData(const mv::Dataset<Points>& reducedPosData)
{
    _reducedPosDataset = reducedPosData;
    if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_2D_POS || _renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_COLOR)
        updataDataTexture();
}

void VolumeRenderer::setMaterialTransitionTexture(const mv::Dataset<Images>& materialTransitionData)
{
    _materialTransitionDataset = materialTransitionData;
    QSize textureDims = _materialTransitionDataset->getImageSize();
    QVector<float> transitionData = QVector<float>(textureDims.width() * textureDims.height() * 4);
    QPair<float, float> scalarDataRange;
    _materialTransitionDataset->getImageScalarData(0, transitionData, scalarDataRange);

    _scalarImageDataRange = scalarDataRange;
    _materialTransitionTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, textureDims.width(), textureDims.height(), 0, GL_RGBA, GL_FLOAT, transitionData.data());
    _materialTransitionTexture.release();
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

    for (int i = 0; i < positionData.size(); i += 2)
    {
        positionData[i] = ((positionData[i] - minX) / rangeX) * (size - 1);
        positionData[i + 1] = ((positionData[i + 1] - minY) / rangeY) * (size - 1);
    }
}


void VolumeRenderer::updataDataTexture()
{
    std::vector<float> textureData;
    QPair<float, float> scalarDataRange;
    mv::Vector3f textureSize;

    if (_volumeDataset.isValid()) {
        if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_FULL || _renderMode == RenderMode::MaterialTransition_FULL) {
            int blockAmount = std::ceil(float(_compositeIndices.size()) / 4.0f) * 4; //Since we always assume textures with 4 dimensions all of which need to be filled
            textureData = std::vector<float>(blockAmount * _volumeDataset->getNumberOfVoxels());
            textureSize = _volumeDataset->getVolumeAtlasData(_compositeIndices, textureData, scalarDataRange);

            // Generate and bind a 3D texture
            _volumeTexture.bind();
            _volumeTexture.setData(textureSize.x, textureSize.y, textureSize.z, textureData, 4);
            _volumeTexture.release(); // Unbind the texture
        }
        else if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_2D_POS || _renderMode == RenderMode::MaterialTransition_2D) {
            if (!_tfDataset.isValid() || !_reducedPosDataset.isValid()) { // _tfTexture is used in the normilize function
                qCritical() << "No position data set";
                return;
            }
            textureData = std::vector<float>(_volumeDataset->getNumberOfVoxels() * 2);
            textureSize = _volumeSize;

            _reducedPosDataset->populateDataForDimensions(textureData, std::vector<int>{0, 1});
            normalizePositionData(textureData);


            // Generate and bind a 3D texture
            _volumeTexture.bind();
            _volumeTexture.setData(textureSize.x, textureSize.y, textureSize.z, textureData, 2);
            _volumeTexture.release(); // Unbind the texture
        }
        else if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_COLOR) {
            if (!_tfDataset.isValid() || !_reducedPosDataset.isValid()){
                qCritical() << "No transfer function data set";
                return;
            }
            int pointAmount = _volumeDataset->getNumberOfVoxels();
            textureData = std::vector<float>(pointAmount * 4);
            //textureData = std::vector<float>(pointAmount * 2);
            textureSize = _volumeSize;
            //TODO get the correct data into textureData 
            std::vector<float> positionData = std::vector<float>(pointAmount * 2);
            _reducedPosDataset->populateDataForDimensions(positionData, std::vector<int>{0, 1});
            normalizePositionData(positionData);
            int width = _tfDataset->getImageSize().width();
            int height = _tfDataset->getImageSize().height();

            for (int i = 0; i < pointAmount; i++)
            {
                int x = positionData[i * 2];
                int y = (positionData[i * 2 + 1]);
                int pixelPos = (y * width + x) * 4;

                //qDebug() << "pixelPos: " << pixelPos;
                textureData[i * 4] = _imageData[pixelPos];
                textureData[(i * 4) + 1] = _imageData[pixelPos + 1];
                textureData[(i * 4) + 2] = _imageData[pixelPos + 2];
                textureData[(i * 4) + 3] = _imageData[pixelPos + 3];
            }
            // Generate and bind a 3D texture
            _volumeTexture.bind();
            _volumeTexture.setData(textureSize.x, textureSize.y, textureSize.z, textureData, 4);
            _volumeTexture.release(); // Unbind the texture
        }
        else if (_renderMode == RenderMode::MIP) {
            textureData = std::vector<float>(_volumeDataset->getNumberOfVoxels());
            textureSize = _volumeDataset->getVolumeAtlasData(std::vector<uint32_t>{ uint32_t(_mipDimension) }, textureData, scalarDataRange, 1);

            // Generate and bind a 3D texture
            _volumeTexture.bind();
            _volumeTexture.setData(textureSize.x, textureSize.y, textureSize.z, textureData, 1);
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
        _settingsChanged = true;
    _compositeIndices = compositeIndices;
}

// Converts render mode string to enum and saves it
void VolumeRenderer::setRenderMode(const QString& renderMode)
{
    RenderMode givenMode;
    if (renderMode == "MultiDimensional Composite Full")
        givenMode = RenderMode::MULTIDIMENSIONAL_COMPOSITE_FULL;
    else if (renderMode == "MultiDimensional Composite 2D Pos")
        givenMode = RenderMode::MULTIDIMENSIONAL_COMPOSITE_2D_POS;
    else if (renderMode == "MultiDimensional Composite Color")
        givenMode = RenderMode::MULTIDIMENSIONAL_COMPOSITE_COLOR;
    else if (renderMode == "1D MIP")
        givenMode = RenderMode::MIP;
    else
        qCritical() << "Unknown render mode";

    if(_renderMode != givenMode)
        _settingsChanged = true;
    _renderMode = givenMode;
}

void VolumeRenderer::setMIPDimension(int mipDimension)
{
    if (_mipDimension != mipDimension)
        _settingsChanged = true;
    _mipDimension = mipDimension;
}

void VolumeRenderer::updateMatrices()
{
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
    shader.uniformMatrix4f("u_modelViewProjection", _mvpMatrix.constData());
    shader.uniformMatrix4f("u_model", _modelMatrix.constData());
    shader.uniform3fv("u_minClippingPlane", 1, &_minClippingPlane);
    shader.uniform3fv("u_maxClippingPlane", 1, &_maxClippingPlane);

    // The actual rendering step
    _vao.bind();
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);

}

// Shared function for all rendertypes, it calculates the ray direction and lengths for each pixel
void VolumeRenderer::renderDirections()
{
    _framebuffer.bind();
    _framebuffer.setTexture(GL_DEPTH_ATTACHMENT, _depthTexture);
    _framebuffer.setTexture(GL_COLOR_ATTACHMENT0, _frontfacesTexture);

    // Set correct settings
    glClear( GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);

    // Render frontfaces into a texture
    _surfaceShader.bind();
    drawDVRRender(_surfaceShader);

    _framebuffer.setTexture(GL_COLOR_ATTACHMENT0, _directionsTexture);

    // Render backfaces and extract direction and length of each ray
    // We count missed rays as very close to the camera since we want to select the furtest away geometry in this renderpass
    glClearDepth(0.0f); 
    glClear(GL_DEPTH_BUFFER_BIT);

    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_GREATER);

    _directionsShader.bind();

    _frontfacesTexture.bind(0);
    _directionsShader.uniform1i("frontfaces", 0);
    if(_useCustomRenderSpace)
        _directionsShader.uniform3fv("dimensions", 1, &_renderSpace);
    else
        _directionsShader.uniform3fv("dimensions", 1, &_volumeSize);
    drawDVRRender(_directionsShader);

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
    _directionsTexture.bind(0);
    _2DCompositeShader.uniform1i("directions", 0);

    _volumeTexture.bind(1);
    _2DCompositeShader.uniform1i("volumeData", 1);

    _tfTexture.bind(2);
    _2DCompositeShader.uniform1i("tfTexture", 2);

    _2DCompositeShader.uniform1f("stepSize", 0.5f);

    if (_useCustomRenderSpace)
        _2DCompositeShader.uniform3fv("dimensions", 1, &_renderSpace);
    else
        _2DCompositeShader.uniform3fv("dimensions", 1, &_volumeSize);
    drawDVRRender(_2DCompositeShader);

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
    _directionsTexture.bind(0);
    _colorCompositeShader.uniform1i("directions", 0);

    _volumeTexture.bind(1);
    _colorCompositeShader.uniform1i("volumeData", 1);

    _colorCompositeShader.uniform1f("stepSize", 0.5f);
    if (_useCustomRenderSpace)
        _colorCompositeShader.uniform3fv("dimensions", 1, &_renderSpace);
    else
        _colorCompositeShader.uniform3fv("dimensions", 1, &_volumeSize);

    drawDVRRender(_colorCompositeShader);

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
    _directionsTexture.bind(0);
    _1DMipShader.uniform1i("directions", 0);

    _volumeTexture.bind(1);
    _1DMipShader.uniform1i("volumeData", 1);

    _1DMipShader.uniform1f("stepSize", 0.5f);
    _1DMipShader.uniform1f("volumeMaxValue", _scalarVolumeDataRange.second);
    _1DMipShader.uniform1i("chosenDim", _mipDimension);
    if (_useCustomRenderSpace)
        _1DMipShader.uniform3fv("dimensions", 1, &_renderSpace);
    else
        _1DMipShader.uniform3fv("dimensions", 1, &_volumeSize);


    drawDVRRender(_1DMipShader);

    // Restore depth clear value
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);
}

void VolumeRenderer::renderMaterialTransition2D()
{
    setDefaultRenderSettings();

    ////Set textures and uniforms
    _materialTransition2DShader.bind();
    _directionsTexture.bind(0);
    _materialTransition2DShader.uniform1i("directions", 0);

    _volumeTexture.bind(1);
    _materialTransition2DShader.uniform1i("volumeData", 1);

    _tfTexture.bind(2);
    _materialTransition2DShader.uniform1i("tfTexture", 2);

    _materialTransitionTexture.bind(3);
    _materialTransition2DShader.uniform1i("materialTexture", 3);

    _materialTransition2DShader.uniform1f("stepSize", 0.5f);

    if (_useCustomRenderSpace)
        _materialTransition2DShader.uniform3fv("dimensions", 1, &_renderSpace);
    else
        _materialTransition2DShader.uniform3fv("dimensions", 1, &_volumeSize);
    drawDVRRender(_materialTransition2DShader);

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
    _surfaceShader.bind();

    updateMatrices();
    renderDirections();

    glBindFramebuffer(GL_FRAMEBUFFER, _defaultFramebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (_volumeDataset.isValid() && _reducedPosDataset.isValid() && _tfDataset.isValid()) {
        if (_settingsChanged) {
            updataDataTexture();
            _settingsChanged = false;
        }
        if (_reducedPosDataset.isValid() && _tfDataset.isValid()) { // This is redundant now
            if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_FULL)
                renderCompositeFull();
            else if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_2D_POS)
                //renderComposite2DPos();
                renderMaterialTransition2D();
            else if (_renderMode == RenderMode::MULTIDIMENSIONAL_COMPOSITE_COLOR)
                renderCompositeColor();
            else if (_renderMode == RenderMode::MIP)
                render1DMip();
            else
                qCritical() << "Unknown render mode";
        }
        else if (_renderMode == RenderMode::MIP) {
            render1DMip();
        }
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
    _directionsTexture.bind(0);
    _framebufferShader.uniform1i("tex", 0);
    drawDVRRender(_framebufferShader);
}

void VolumeRenderer::destroy()
{
    _vao.destroy();
    _vbo.destroy();
    _ibo.destroy();
    _surfaceShader.destroy();
    _framebufferShader.destroy();
}

