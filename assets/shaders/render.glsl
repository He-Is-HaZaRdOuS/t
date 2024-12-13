#version 430

layout (std430, binding = 1) readonly buffer dvrLayout
{
    vec4 dvrBuffer[];
};

in vec2 fragTexCoord;
out vec4 finalColor;
uniform vec2 resolution;
uniform float brightness;

void main()
{
    ivec2 coords = ivec2(fragTexCoord * resolution);
    vec4 color = dvrBuffer[coords.x + coords.y * int(resolution.x)];
    finalColor = vec4(color.rgb, color.a * brightness);
}