#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform vec4 lineColor;
uniform vec4 fillColor;
uniform float thickness;

void main()
{
    float u = fragTexCoord.x;
    float v = fragTexCoord.y;
    vec3 bary = vec3(u, v, 1.0 - u - v);

    vec3 dIndex = fwidth(bary);
    vec3 a3 = smoothstep(vec3(0.0), dIndex * thickness, bary);

    float edgeFactor = min(min(a3.x, a3.y), a3.z);

    finalColor = mix(lineColor, fillColor, edgeFactor);
    
    if (finalColor.a < 0.01) {
        discard;
    }
}
