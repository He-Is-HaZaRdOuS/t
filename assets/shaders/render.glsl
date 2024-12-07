#version 430

layout (std430, binding = 1) readonly buffer dvrLayout
{
    float dvrBuffer[];
};

in vec2 fragTexCoord;
out vec4 finalColor;
uniform vec2 resolution;
uniform float brightness;

void main()
{
    ivec2 coords = ivec2(fragTexCoord * resolution);
    float alpha = dvrBuffer[coords.x + coords.y * uvec2(resolution).x];
    finalColor = vec4(1.0f, 1.0f, 1.0f, alpha * brightness);
}