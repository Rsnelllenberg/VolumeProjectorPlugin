#version 330 core

layout(location = 0) in vec4 position;
layout(location = 1) in float color;

uniform mat4 projMatrix;
uniform mat4 viewMatrix;
uniform mat4 modelMatrix;

out float v_Color;

void main() {
    gl_Position = projMatrix * viewMatrix * modelMatrix * position;
    
    v_Color = color;
}
