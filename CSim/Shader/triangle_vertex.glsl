#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 uMVP; // The Model-View-Projection matrix from the camera

void main() { 
    gl_Position = uMVP * vec4(aPos, 1.0f);
    TexCoord = aTexCoord;
}
