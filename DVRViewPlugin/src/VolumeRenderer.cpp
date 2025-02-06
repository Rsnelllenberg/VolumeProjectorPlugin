#include "VolumeRenderer.h"
#include <QImage>
#include <random>
#include <QOpenGLWidget>

void VolumeRenderer::init()
{
    qDebug() << "Initializing VolumeRenderer";
    initializeOpenGLFunctions();

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    _volumeTexture.create();
    _volumeTexture.initialize();

    _tfTexture.create();
    _tfTexture.bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

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
    loaded &= _noTFCompositeShader.loadShaderFromFile(":shaders/Surface.vert", ":shaders/NoTFcomposite.frag");
    loaded &= _1DMipShader.loadShaderFromFile(":shaders/Surface.vert", ":shaders/1DMip.frag");
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
    QVector<float> textureData(textureDims.width() * textureDims.height() * 4);
    QPair<float, float> scalarDataRange;
    _tfDataset->getImageScalarData(0, textureData, scalarDataRange);

    _scalarImageDataRange = scalarDataRange;

    _tfTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, textureDims.width(), textureDims.height(), 0, GL_RGBA, GL_FLOAT, textureData.data());
    _tfTexture.release();
}

void VolumeRenderer::updataDataTexture()
{
    std::vector<float> textureData;
    if (_renderMode == "MultiDimensional Composite") {
        int blockAmount = std::ceil(_compositeIndices.size() / 4.0f) * 4; //Since we always assume textures with 4 dimensions all of which need to be filled
        textureData = std::vector<float>(blockAmount * _volumeDataset->getNumberOfVoxels());
    }
    else if (_renderMode == "1D MIP")
        textureData = std::vector<float>(_volumeDataset->getNumberOfVoxels());
    else
        qCritical() << "Unknown render mode";

    QPair<float, float> scalarDataRange;
    mv::Vector3f textureSize;
    if (_renderMode == "MultiDimensional Composite") {
            textureSize = _volumeDataset->getVolumeAtlasData(_compositeIndices, textureData, scalarDataRange);

            // Generate and bind a 3D texture
            _volumeTexture.bind();
            _volumeTexture.setData(textureSize.x, textureSize.y, textureSize.z, textureData, 4);
            _volumeTexture.release(); // Unbind the texture
    }
    else if (_renderMode == "1D MIP") {
        textureSize = _volumeDataset->getVolumeAtlasData(std::vector<uint32_t>{ uint32_t(_mipDimension) }, textureData, scalarDataRange, 1);

        // Generate and bind a 3D texture
        _volumeTexture.bind();
        _volumeTexture.setData(textureSize.x, textureSize.y, textureSize.z, textureData, 1);
        _volumeTexture.release(); // Unbind the texture
    }
    else 
        qCritical() << "Unknown render mode";
        
    _scalarVolumeDataRange = scalarDataRange;



}

void VolumeRenderer::setTransferfunction(const QImage& colormap)
{
    // TODO: Implement transfer function
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

void VolumeRenderer::setCompositeIndices(std::vector<std::uint32_t> compositeIndices)
{
    if (_compositeIndices != compositeIndices)
        _settingsChanged = true;
    _compositeIndices = compositeIndices;
}

void VolumeRenderer::setRenderMode(const QString& renderMode)
{
    if(_renderMode != renderMode)
        _settingsChanged = true;
    _renderMode = renderMode;
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
    _directionsShader.uniform3fv("dimensions", 1, &_volumeSize);
    drawDVRRender(_directionsShader);

    // Restore depth clear value
    glClearDepth(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);

    _framebuffer.release();
}

// Render the volume using a dummy composite shader that does not require a transfer function (mostly for testing purposes)
void VolumeRenderer::renderCompositeNoTF()
{
    setDefaultRenderSettings();

    ////Set textures and uniforms
    _noTFCompositeShader.bind();
    _directionsTexture.bind(0);
    _noTFCompositeShader.uniform1i("directions", 0);

    _volumeTexture.bind(1);
    _noTFCompositeShader.uniform1i("volumeData", 1);

    _noTFCompositeShader.uniform1f("stepSize", 0.5f);
    _noTFCompositeShader.uniform3fv("brickSize", 1, &_volumeSize);
    drawDVRRender(_noTFCompositeShader);

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
    _1DMipShader.uniform3fv("brickSize", 1, &_volumeSize);
    _1DMipShader.uniform1f("volumeMaxValue", _scalarVolumeDataRange.second);
    _1DMipShader.uniform1i("chosenDim", _mipDimension);

    drawDVRRender(_1DMipShader);

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

    if (_volumeDataset.isValid()) {
        if (_settingsChanged) {
            updataDataTexture();
            _settingsChanged = false;
        }
        if (_renderMode == "MultiDimensional Composite")
            renderCompositeNoTF();
        else if (_renderMode == "1D MIP")
            render1DMip();
        else
            qCritical() << "Unknown render mode";
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

