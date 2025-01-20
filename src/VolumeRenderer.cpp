#include "VolumeRenderer.h"
#include <QImage>
#include <random>
#include <QOpenGLWidget>

void VolumeRenderer::init()
{
    initializeOpenGLFunctions();

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    _voxelBox.init(50, 50, 50);

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

void VolumeRenderer::setData(const std::vector<float>& spatialData, const std::vector<float>& valueData, int numValueDimensions)
{
    _voxelBox.setData(spatialData, valueData, numValueDimensions);

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

void VolumeRenderer::reloadShader()
{
    _surfaceShader.loadShaderFromFile(":shaders/Surface.vert", ":shaders/Surface.frag"); //TODO: add other shaders
    qDebug() << "Shaders reloaded";
}

void VolumeRenderer::updateMatrices()
{
    // Create the model-view-projection matrix
    mv::Vector3f dims = _voxelBox.getDims();

    QMatrix4x4 modelMatrix;
    modelMatrix.scale(dims.x, dims.y, dims.z);
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
    qDebug() << "Rendering directions";

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

    mv::Vector3f dims = _voxelBox.getDims();

    _frontfacesTexture.bind(0);
    _directionsShader.uniform1i("frontfaces", 0);
    _directionsShader.uniform3fv("dimensions", 1, &dims);
    drawDVRRender(_directionsShader);

    // Restore depth clear value
    glClearDepth(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);

    _framebuffer.release();
}

void VolumeRenderer::renderCompositeNoTF(mv::Texture3D& volumeTexture)
{
    //_framebuffer.bind();
    //_framebuffer.setTexture(GL_COLOR_ATTACHMENT0, _renderTexture);
    //_framebuffer.setTexture(GL_DEPTH_ATTACHMENT, _depthTexture);

    // Clear the depth buffer for the next render pass
    glClearDepth(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);

    ////Set textures and uniforms
    _noTFCompositeShader.bind();
    _directionsTexture.bind(0);
    _noTFCompositeShader.uniform1i("directions", 0);
    mv::Vector3f brickLayout = _voxelBox.getBrickLayout();

    volumeTexture.bind(1);
    _noTFCompositeShader.uniform1i("volumeData", 1);

    _noTFCompositeShader.uniform1f("stepSize", 0.5f);
    _noTFCompositeShader.uniform3fv("brickLayout", 1, &brickLayout);
    drawDVRRender(_noTFCompositeShader);

    // Restore depth clear value
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);

    //_framebuffer.release();
}


void VolumeRenderer::render()
{
    _surfaceShader.bind();

    updateMatrices();
    renderDirections();

    glBindFramebuffer(GL_FRAMEBUFFER, _defaultFramebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);

    mv::Texture3D& volumeTexture = _voxelBox.getVolumeTexture();
    if (_voxelBox.hasData())
        renderCompositeNoTF(volumeTexture);
    else {
        _directionsTexture.bind(0);
        _framebufferShader.uniform1i("tex", 0);
        drawDVRRender(_framebufferShader);
    }
    qDebug() << "Rendered";
}

void VolumeRenderer::destroy()
{
    _vao.destroy();
    _vbo.destroy();
    _ibo.destroy();
    _surfaceShader.destroy();
    _framebufferShader.destroy();
}

const VoxelBox& VolumeRenderer::getVoxelBox() const
{
    return _voxelBox;
}

