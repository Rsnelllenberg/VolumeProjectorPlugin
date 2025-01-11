#version 330
layout(location = 0) in vec3 pos;

uniform mat4 u_modelViewProjection;

void main() {
	gl_Position = u_modelViewProjection * vec4(pos, 1.0);
}