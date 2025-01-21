#version 330
layout(location = 0) in vec3 pos;

uniform mat4 u_modelViewProjection;
uniform mat4 u_model; 
uniform vec3 u_minClippingPlane;
uniform vec3 u_maxClippingPlane;

out vec3 u_color;
out vec3 worldPos; 



void main() {
    vec3 actualPos = clamp(pos,u_minClippingPlane, u_maxClippingPlane);
    worldPos = (u_model * vec4(actualPos, 1.0)).xyz;
    gl_Position = u_modelViewProjection * vec4(actualPos, 1.0);
    u_color = actualPos;
}
