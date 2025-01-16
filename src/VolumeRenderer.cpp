#include "VolumeRenderer.h"
#include <QImage>
#include <random>
#include <QOpenGLWidget>

void VolumeRenderer::init()
{
    initializeOpenGLFunctions();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

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

    _noTFCompositeShader.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, renderSize.width(), renderSize.height(), 0, GL_RGBA, GL_FLOAT, nullptr);
    _noTFCompositeShader.release();
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

void VolumeRenderer::reloadShader()
{
    _surfaceShader.loadShaderFromFile(":shaders/Surface.vert", ":shaders/Surface.frag"); //TODO: add other shaders
    qDebug() << "Shaders reloaded";
}

void VolumeRenderer::updateMatrices()
{
    // Create the model-view-projection matrix
    _mvpMatrix.setToIdentity();
    _mvpMatrix.perspective(45.0f, _camera.getAspect(), 0.1f, 400.0f);
    _mvpMatrix *= _camera.getViewMatrix();

    mv::Vector3f dims = _voxelBox.getDims();
    _mvpMatrix.scale(dims.x, dims.y, dims.z);
}

void VolumeRenderer::drawDVRRender(mv::ShaderProgram& shader)
{
    shader.uniformMatrix4f("u_modelViewProjection", _mvpMatrix.constData());
    shader.uniformMatrix4f("u_model", _modelMatrix.constData());

    // The actual rendering step
    _vao.bind();
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
    _vao.release();
}

// Shared function for all rendertypes, it calculates the ray direction and lengths for each pixel
void VolumeRenderer::renderDirections()
{
    qDebug() << "Rendering directions";
    QSize renderResolution = _screenSize;
    mv::Vector3f dims = _voxelBox.getDims();

    _frontfacesTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, renderResolution.width(), renderResolution.height(), 0, GL_RGBA, GL_FLOAT, nullptr);

    _depthTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, renderResolution.width(), renderResolution.height(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    _framebuffer.bind();
    _framebuffer.setTexture(GL_DEPTH_ATTACHMENT, _depthTexture);
    _framebuffer.setTexture(GL_COLOR_ATTACHMENT0, _frontfacesTexture);

    // Clear buffers
    glClear( GL_DEPTH_BUFFER_BIT);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_BLEND);

    // Render frontfaces into a texture
    _surfaceShader.bind();
    drawDVRRender(_surfaceShader);
    
    _directionsTexture.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, renderResolution.width(), renderResolution.height(), 0, GL_RGBA, GL_FLOAT, nullptr);

    _framebuffer.setTexture(GL_COLOR_ATTACHMENT0, _directionsTexture);

    // Clear the depth buffer for the next render pass
    glClearDepth(0.0f);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Render backfaces and extract direction and length of each ray
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_GREATER);

    _directionsShader.bind();

    glActiveTexture(GL_TEXTURE0);
    _frontfacesTexture.bind(0);
    _directionsShader.uniform1i("frontfaces", 0);
    _directionsShader.uniform3fv("dimensions", 1, &dims);
    drawDVRRender(_directionsShader);

    // Restore depth clear value
    glClearDepth(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);

    // Unbind the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
    if (_voxelBox.hasData()) {
        qDebug() << "Volume texture render started";
        glActiveTexture(GL_TEXTURE0);
        _directionsTexture.bind(0);
        _noTFCompositeShader.uniform1i("directions", 0);

        glActiveTexture(GL_TEXTURE1);
        volumeTexture.bind(1);
        _noTFCompositeShader.uniform1i("volumeData", 0);

        _noTFCompositeShader.uniform1f("stepSize", 1.0f);

        drawDVRRender(_noTFCompositeShader);
    }
    else {
        qDebug() << "Direction texture rendered";
        glActiveTexture(GL_TEXTURE0);
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

