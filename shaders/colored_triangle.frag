#version 450

// input 
layout(location = 0) in vec3 inColor;

// output
layout (location = 0) out vec4 outFragColor;

void main() {
    outFragColor = vec4(inColor, 1.0); // Set the output color with full opacity
}