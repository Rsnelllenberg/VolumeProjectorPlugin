#pragma once

#include "graphics/Shader.h"
#include "graphics/Vector3f.h"
#include "graphics/Vector2f.h"
#include "graphics/Framebuffer.h"
#include "graphics/Texture.h"

#include <QOpenGLFunctions_3_3_Core>
#include <QMatrix4x4>

#include <vector>

/**
 * OpenGL Volume Renderer
 * This class provides a pure OpenGL renderer for volume data
 *
 * @author Julian Thijssen
 */
class VolumeRenderer : public QOpenGLFunctions_3_3_Core
{
public:
    void setTexels(int width, int height, int depth, std::vector<float>& texels);
    void setData(std::vector<float>& data);
    void setColors(std::vector<float>& colors);
    void setColormap(const QImage& colormap);
    void setCursorPoint(mv::Vector3f cursorPoint);
    void reloadShader();

    void init();
    void resize(int w, int h);

    void render(GLuint framebuffer, mv::Vector3f camPos, mv::Vector2f camAngle, float aspect);

private:
    mv::Framebuffer _framebuffer;
    mv::Texture2D _colorAttachment;
    //GLuint _texture;

    mv::ShaderProgram _volumeShaderProgram;
    mv::ShaderProgram _pointsShaderProgram;
    mv::ShaderProgram _framebufferShaderProgram;

    GLuint vao;
    GLuint vbo;
    GLuint cbo;
    int _numPoints = 0;

    GLuint _cursorVao;
    GLuint _cursorVbo;
    mv::Vector3f _cursorPoint;

    bool _hasColors = false;

    mv::Texture2D _colormap;

    QMatrix4x4 _projMatrix;
    QMatrix4x4 _viewMatrix;
    QMatrix4x4 _modelMatrix;
};
