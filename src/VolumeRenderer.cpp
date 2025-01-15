#include "VolumeRenderer.h"
#include <QImage>
#include <random>
#include <QOpenGLWidget>

void VolumeRenderer::init()
{
    initializeOpenGLFunctions();

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // initialize textures
    glGenTextures(1, &_frontfacesTexture);
    glBindTexture(GL_TEXTURE_2D, _frontfacesTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenTextures(1, &_directionsTexture);
    glBindTexture(GL_TEXTURE_2D, _directionsTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    // The general framebuffer with depth component
    glGenFramebuffers(1, &_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

    // Create a texture for the depth buffer
    glGenTextures(1, &_depthTexture);
    glBindTexture(GL_TEXTURE_2D, _depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 720, 720, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Attach the depth buffer to the framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture, 0);

    // Attach the existing color texture

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _frontfacesTexture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Initialize the volume shader program
    bool loaded = true;
    loaded &= _surfaceShader.loadShaderFromFile(":shaders/Surface.vert", ":shaders/Surface.frag");
    loaded &= _directionsShader.loadShaderFromFile(":shaders/Surface.vert", ":shaders/Directions.frag"); 
    loaded &= _framebufferShader.loadShaderFromFile(":shaders/Quad.vert", ":shaders/Texture.frag");

    if (!loaded) {
        qCritical() << "Failed to load one of the Volume Renderer shaders";
    }
    else {
        qDebug() << "Volume Renderer shaders loaded";
    }

    // Initialize a cube mesh 
    const std::array vertices{
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
}

void VolumeRenderer::resize(QSize renderSize)
{
    _screenSize = renderSize;
}

void VolumeRenderer::setData(const std::vector<mv::Vector3f>& spatialData, const std::vector<std::vector<float>>& valueData)
{
    //_voxelBox.setData(spatialData, valueData);
    //_numPoints = _voxelBox.getBoxSize();

    //// Retrieve dimensions from _voxelBox
    //mv::Vector3f dims = _voxelBox.getDims();
    //int width = static_cast<int>(dims.x);
    //int height = static_cast<int>(dims.y);
    //int depth = static_cast<int>(dims.z);

    //// Generate and bind a 3D texture
    //_volumeTexture->bind();

    //// Fill the texture with data from _voxelBox
    //const std::vector<Voxel>& voxels = _voxelBox.getVoxels();
    //std::vector<float> textureData(width * height * depth, 0.0f);

    //for (const Voxel& voxel : voxels) {
    //    int x = static_cast<int>(voxel.position.x);
    //    int y = static_cast<int>(voxel.position.y);
    //    int z = static_cast<int>(voxel.position.z);
    //    size_t index = x + y * width + z * width * height;
    //    if (index < textureData.size()) {
    //        textureData[index] = voxel.values.empty() ? 0.0f : voxel.values[0]; // Assuming the first value is used
    //    }
    //}

    //_volumeTexture->setData(QOpenGLTexture::Red, QOpenGLTexture::Float32, textureData.data());
    //_volumeTexture->release(); // Unbind the texture
}

void VolumeRenderer::setTransferfunction(const QImage& colormap)
{
    // TODO: Implement transfer function
}

void VolumeRenderer::setCamera(const TrackballCamera& camera)
{
    _camera = camera;
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
    qDebug() << "Rendering directions 1";
    // The actual rendering step
    _vao.bind();
    qDebug() << "Rendering directions 1.1";
    glDrawArrays(GL_TRIANGLES, 0, 36);
    qDebug() << "Rendering directions 1.2"; 
}

// Shared function for all rendertypes, it calculates the ray direction and lengths for each pixel
void VolumeRenderer::renderDirections()
{
    qDebug() << "Rendering directions";
    QSize renderResolution = _screenSize;
    mv::Vector3f dims = _voxelBox.getDims();

    glBindTexture(GL_TEXTURE_2D, _depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, renderResolution.width(), renderResolution.height(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    
    glBindTexture(GL_TEXTURE_2D, _frontfacesTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, renderResolution.width(), renderResolution.height(), 0, GL_RGBA, GL_FLOAT, nullptr);

    //Bind the framebuffer and attach _frontfaces texture
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _frontfacesTexture, 0);
     
    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // Render frontfaces into a texture
    _surfaceShader.bind();
    qDebug() << "Rendering directions 2";
    drawDVRRender(_surfaceShader);
    
    // Update texture for the directions
    glBindTexture(GL_TEXTURE_2D, _directionsTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, renderResolution.width(), renderResolution.height(), 0, GL_RGBA, GL_FLOAT, nullptr);

    // Attach direction texture to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _directionsTexture, 0);

    // Clear the depth buffer for the next render pass
    glClearDepth(0.0f);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Render backfaces and extract direction and length of each ray
    glDisable(GL_CULL_FACE);
    glDepthFunc(GL_GREATER);

    // Bind shader and set its uniformes
    _directionsShader.bind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _frontfacesTexture);
    _directionsShader.uniform1i("frontfaces", 0);

    _directionsShader.uniform3fv("dimensions", 1, &dims);
    drawDVRRender(_directionsShader);
    qDebug() << "Rendering directions 3";
    // Restore depth clear value
    glClearDepth(1.0f);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDepthFunc(GL_LEQUAL);

    // Unbind the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


void VolumeRenderer::render()
{
    qDebug() << "Rendering volume data";
    _surfaceShader.bind();

    updateMatrices();
    //renderDirections();
    qDebug() << "Directions rendered";

    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, _directionsTexture);
    //_surfaceShader.uniform1i("givenTexture", 0);

    drawDVRRender(_surfaceShader);
    
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

