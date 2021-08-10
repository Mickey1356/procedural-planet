#version 460 core

layout (location = 0) in vec3 aPos;

uniform mat4 vp;
uniform mat4 model;
uniform float radius;

void main() {
    gl_Position = vp * model * vec4(normalize(aPos) * radius, 1.0);
    gl_PointSize = 10.0;
}