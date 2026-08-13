#version 330 core
layout(location = 0) in vec4 posAttr;
out vec4 pos;
uniform mat4 matrix;
void main() {
    pos = posAttr;
    gl_Position = matrix * posAttr;
}
