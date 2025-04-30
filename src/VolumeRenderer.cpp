#include "VolumeRenderer.h"

#include <QImage>

#include <random>

void VolumeRenderer::setTexels(int width, int height, int depth, std::vector<float>& texels)
{

}

void VolumeRenderer::setData(std::vector<float>& data)
{ //TODO update this to a 3D texture
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &cbo);
    glBindBuffer(GL_ARRAY_BUFFER, cbo);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 0, 0);
    //glEnableVertexAttribArray(1);

    _numPoints = data.size() / 3;
}

void VolumeRenderer::setColors(std::vector<float>& colors)
{
    glBindVertexArray(vao);
    qDebug() << colors.size();
    glBindBuffer(GL_ARRAY_BUFFER, cbo);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), colors.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);

    _hasColors = true;
}

void VolumeRenderer::setColormap(const QImage& colormap)
{
    _colormap.loadFromImage(colormap);
    qDebug() << "Colormap is set!";
}

void VolumeRenderer::setCursorPoint(mv::Vector3f cursorPoint)
{
    _cursorPoint = cursorPoint;
    qDebug() << _cursorPoint.x << _cursorPoint.y << _cursorPoint.z;
}

void VolumeRenderer::reloadShader()
{
	_pointsShaderProgram.loadShaderFromFile(":shaders/", ":shaders/"); // TODO use correct path
    qDebug() << "Shaders reloaded";
}

void VolumeRenderer::init()
{
    initializeOpenGLFunctions();

    glClearColor(22 / 255.0f, 22 / 255.0f, 22 / 255.0f, 1.0f);

    // Make float buffer to support low alpha blending
    _colorAttachment.create();
    _colorAttachment.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 1, 1, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    _framebuffer.create();
    _framebuffer.bind();
    _framebuffer.addColorTexture(0, &_colorAttachment);
    _framebuffer.validate();

    bool loaded = true;
    loaded &= _volumeShaderProgram.loadShaderFromFile("volume.vert", "volume.frag");
    loaded &= _pointsShaderProgram.loadShaderFromFile(":shaders/points.vert", ":shaders/points.frag");
    loaded &= _framebufferShaderProgram.loadShaderFromFile(":shaders/Quad.vert", ":shaders/Texture.frag");

    if (!loaded) {
        qCritical() << "Failed to load one of the Volume Renderer shaders";
    }

    //glGenTextures(1, &_texture);

    glGenVertexArrays(1, &vao);

    qDebug() << "Initialized volume renderer";

    glEnable(GL_BLEND);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

    glPointSize(3);

    glGenVertexArrays(1, &_cursorVao);
    glBindVertexArray(_cursorVao);

    glGenBuffers(1, &_cursorVbo);
    glBindBuffer(GL_ARRAY_BUFFER, _cursorVbo);
    glBufferData(GL_ARRAY_BUFFER, 0 * sizeof(float), nullptr, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    /////////////
    //int width = 200;
    //int height = 400;
    //int depth = 100;
    //std::vector<float> texels(width * height * depth, 0);

    //std::default_random_engine generator;
    //std::uniform_real_distribution<float> distribution(0, 1);

    //for (int z = 0; z < depth; z++)
    //{
    //    for (int x = 0; x < width; x++)
    //    {
    //        for (int y = 0; y < height; y++)
    //        {
    //            float rand = distribution(generator);

    //            //texels[x * height * depth + y * depth + z] = rand;
    //            if (z > 25 && z < 75)
    //                texels[z * width * height + x * height + y] = 1;
    //        }
    //    }
    //}

    //glBindTexture(GL_TEXTURE_2D_ARRAY, _texture);
    //glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R32F, width, height, depth);
    //glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, width, height, depth, GL_RED, GL_FLOAT, texels.data());
    //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    //glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void VolumeRenderer::resize(int w, int h)
{
    qDebug() << "Resize called";
    _colorAttachment.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);

    glViewport(0, 0, w, h);
}

void VolumeRenderer::render(GLuint framebuffer, mv::Vector3f camPos, mv::Vector2f camAngle, float aspect)
{
    _framebuffer.bind();
    glDrawBuffer(GL_COLOR_ATTACHMENT0);

    glEnable(GL_BLEND);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

    glClear(GL_COLOR_BUFFER_BIT);

#ifdef VOLUME
    _volumeShaderProgram.bind();

    glActiveTexture(GL_TEXTURE0);
    _volumeShaderProgram.uniform1i("tex", 0);

    glBindTexture(GL_TEXTURE_2D_ARRAY, _texture);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
#else
    _pointsShaderProgram.bind();

    _projMatrix.setToIdentity();
    float fovyr = 1.57079633;
    float zNear = 0.1f;
    float zFar = 100;
    _projMatrix.data()[0] = (float)(1 / tan(fovyr / 2)) / aspect;
    _projMatrix.data()[5] = (float)(1 / tan(fovyr / 2));
    _projMatrix.data()[10] = (zNear + zFar) / (zNear - zFar);
    _projMatrix.data()[11] = -1;
    _projMatrix.data()[14] = (2 * zNear * zFar) / (zNear - zFar);
    _projMatrix.data()[15] = 0;

    //_projMatrix.data()[12] = 1;

    _viewMatrix.setToIdentity();
    //_viewMatrix.rotate(camAngle.y * (180 / 3.14159), 0, 1, 0);
    //_viewMatrix.rotate(camAngle.x * (180 / 3.14159), 1, 0, 0);
    //_viewMatrix.translate(-camPos.x, -camPos.y, -camPos.z);
    _viewMatrix.lookAt(QVector3D(camPos.x, camPos.y, camPos.z), QVector3D(0, 0, 0), QVector3D(0, 1, 0));

    _modelMatrix.setToIdentity();

    //for (int i = 0; i < 16; i++)
    //    qDebug() << _modelMatrix.data()[i];
    _pointsShaderProgram.uniformMatrix4f("projMatrix", _projMatrix.data());
    _pointsShaderProgram.uniformMatrix4f("viewMatrix", _viewMatrix.data());
    _pointsShaderProgram.uniformMatrix4f("modelMatrix", _modelMatrix.data());

    glPointSize(3);
    glBindVertexArray(vao);

    _pointsShaderProgram.uniform1i("hasColors", false);

    glDrawArrays(GL_POINTS, 0, _numPoints);

    if (_hasColors)
    {
        _pointsShaderProgram.uniform1i("hasColors", _hasColors);

        if (_colormap.isCreated())
        {
            _colormap.bind(0);
            _pointsShaderProgram.uniform1i("colormap", 0);
        }

        glDrawArrays(GL_POINTS, 0, _numPoints);
    }

    // Draw the cursor
    _pointsShaderProgram.uniform1i("isCursor", 1);
    glBindVertexArray(_cursorVao);
    glBindBuffer(GL_ARRAY_BUFFER, _cursorVbo);
    glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(float), &_cursorPoint, GL_STATIC_DRAW);

    glEnable(GL_POINT_SMOOTH);
    glPointSize(15);
    glDrawArrays(GL_POINTS, 0, 1);
    _pointsShaderProgram.uniform1i("isCursor", 0);
    glDisable(GL_POINT_SMOOTH);

    ///////////////////////////////////////////////////////////////////////
    // Draw the color framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glDrawBuffer(GL_BACK);

    glDisable(GL_BLEND);

    glClear(GL_COLOR_BUFFER_BIT);

    _framebufferShaderProgram.bind();

    _colorAttachment.bind(0);
    _framebufferShaderProgram.uniform1i("tex", 0);

    glDrawArrays(GL_TRIANGLES, 0, 3);
#endif
}
