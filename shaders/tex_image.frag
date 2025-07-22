#version 450

// input 
layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inUV;

// output
layout (location = 0) out vec4 outFragColor;


// texture
layout (set = 0, binding = 0) uniform sampler2D texImage;

void main() {
    outFragColor = texture(texImage, inUV); // Set the output color with full opacity
}