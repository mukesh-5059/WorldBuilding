#version 330

in vec3 fragPosition;
in vec4 fragColor;

out vec4 finalColor;

uniform vec4 lineColor;
uniform vec4 fillColor;
uniform float thickness;

void main()
{
    finalColor = fillColor * fragColor;
}
