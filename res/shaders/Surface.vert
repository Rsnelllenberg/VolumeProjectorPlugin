#version 330
layout(location = 0) in vec3 pos;

uniform mat4 u_modelViewProjection;
uniform mat4 u_model; 

out vec3 u_color;
out vec3 worldPos; 



void main() {
    vec3 actualPos = pos;
    worldPos = (u_model * vec4(actualPos, 1.0)).xyz;
    gl_Position = u_modelViewProjection * vec4(actualPos, 1.0);
    u_color = actualPos;
}
