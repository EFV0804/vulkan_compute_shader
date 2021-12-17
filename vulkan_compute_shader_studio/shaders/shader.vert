#version 450

// Output colors for vertex shader
layout(location = 0) out vec3 fragColor;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    fragColor = inColor;
}
