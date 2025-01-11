#include "VolumeRenderer.h"
#include <QImage>
#include <random>

void VolumeRenderer::init()
{
    initializeOpenGLFunctions();

    glClearColor(22 / 255.0f, 22 / 255.0f, 22 / 255.0f, 1.0f);

    mv::Vector3f dims = _voxelBox.getDims();
    int width = static_cast<int>(dims.x);
    int height = static_cast<int>(dims.y);
    int depth = static_cast<int>(dims.z);

    // Generate and bind the 3D texture
    glGenTextures(1, &_volumeTexture);
    glBindTexture(GL_TEXTURE_3D, _volumeTexture);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, width, height, depth, 0, GL_RED, GL_FLOAT, nullptr);

    glBindTexture(GL_TEXTURE_3D, 0); // Unbind the texture

    // Initialize the volume shader program
    bool loaded = true;
    loaded &= _volumeShaderProgram.loadShaderFromFile(":shaders/volume.vert", ":shaders/volume.frag"); // TODO: use correct path
    loaded &= _framebufferShaderProgram.loadShaderFromFile(":shaders/Quad.vert", ":shaders/Texture.frag");

    if (!loaded) {
        qCritical() << "Failed to load one of the Volume Renderer shaders";
    }
    else {
        qDebug() << "Volume Renderer shaders loaded";
    }

    // Initialize the frame + framebuffer
    //_generatedFrame.create();
    //_generatedFrame.bind();
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 800, 600, 0, GL_RGBA, GL_FLOAT, nullptr); // Example dimensions

    //_framebuffer.create();
    //_framebuffer.bind();
    //_framebuffer.addColorTexture(GL_COLOR_ATTACHMENT0, &_generatedFrame);
    //_framebuffer.validate();
    //_framebuffer.release();

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
    glGenVertexArrays(1, &_vao);
    glBindVertexArray(_vao);

    glGenBuffers(1, &_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

    glGenBuffers(1, &_ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void VolumeRenderer::setData(std::vector<mv::Vector3f>& spatialData, std::vector<std::vector<float>>& valueData)
{
    _voxelBox.setData(spatialData, valueData);
    _numPoints = _voxelBox.getBoxSize();

    // Retrieve dimensions from _voxelBox
    mv::Vector3f dims = _voxelBox.getDims();
    int width = static_cast<int>(dims.x);
    int height = static_cast<int>(dims.y);
    int depth = static_cast<int>(dims.z);

    // Generate and bind a 3D texture
    glBindTexture(GL_TEXTURE_3D, _volumeTexture);

    // Fill the texture with data from _voxelBox
    const std::vector<Voxel>& voxels = _voxelBox.getVoxels();
    std::vector<float> textureData(width * height * depth, 0.0f);

    for (const Voxel& voxel : voxels) {
        int x = static_cast<int>(voxel.position.x);
        int y = static_cast<int>(voxel.position.y);
        int z = static_cast<int>(voxel.position.z);
        size_t index = x + y * width + z * width * height;
        if (index < textureData.size()) {
            textureData[index] = voxel.values.empty() ? 0.0f : voxel.values[0]; // Assuming the first value is used
        }
    }

    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, width, height, depth, 0,  GL_RED, GL_FLOAT, textureData.data());
    glBindTexture(GL_TEXTURE_3D, 0); // Unbind the texture
}

void VolumeRenderer::setTransferfunction(const QImage& colormap)
{
    // TODO: Implement transfer function
}

void VolumeRenderer::reloadShader()
{
    _volumeShaderProgram.loadShaderFromFile(":shaders/volume.vert", ":shaders/volume.frag"); // TODO use correct path
    qDebug() << "Shaders reloaded";
}

void VolumeRenderer::resize(int w, int h)
{
    //qDebug() << "Resize called";
    //_generatedFrame.bind();
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);

    //glViewport(0, 0, w, h);
    //_generatedFrame.release();
}

void VolumeRenderer::render(GLuint framebuffer, TrackballCamera camera)
{
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _volumeShaderProgram.bind();

    // Create the model-view-projection matrix
    QMatrix4x4 mvpMatrix;
    mvpMatrix.perspective(45.0f, camera.getAspect(), 0.1f, 100.0f);
    mvpMatrix *= camera.getViewMatrix();
    mv::Vector3f dims = _voxelBox.getDims();
    mvpMatrix.scale(dims.x, dims.y, dims.z);

    // Set the MVP matrix uniform in the shader
    _volumeShaderProgram.uniformMatrix4f("u_modelViewProjection", mvpMatrix.constData());

    // The actual rendering step
    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);

    // clean up
    glBindVertexArray(0);
    _volumeShaderProgram.release();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


