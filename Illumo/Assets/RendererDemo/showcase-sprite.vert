#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aUv;

out vec4 ourColor;
out vec2 ourUv;

uniform int uUsePixels;
uniform vec2 u_resolution;
uniform mat4 uMVP;

void main()
{
    if (uUsePixels != 0) {
        float x = (aPos.x / u_resolution.x) * 2.0 - 1.0;
        float y = 1.0 - (aPos.y / u_resolution.y) * 2.0;
        gl_Position = vec4(x, y, aPos.z, 1.0);
    } else {
        gl_Position = uMVP * vec4(aPos, 1.0);
    }
    ourColor = aColor;
    ourUv = aUv;
}
