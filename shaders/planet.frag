#version 450

layout (location = 0) in vec2 TexCoord;
layout (location = 0) out vec4 color;

void main()
{
    color = vec4(TexCoord, 0.0, 1.0);
}
