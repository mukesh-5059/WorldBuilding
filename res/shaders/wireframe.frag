#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;

out vec4 finalColor;

uniform vec4 lineColor;
uniform vec4 fillColor;
uniform float thickness;

void main()
{
    float uBary = fragTexCoord.x;
    float vBary = fragTexCoord.y;
    vec3 bary = vec3(uBary, vBary, 1.0 - uBary - vBary);

    vec3 dIndex = fwidth(bary);
    vec3 a3 = smoothstep(vec3(0.0), dIndex * thickness, bary);
    
    float edgeFactor = min(min(a3.x, a3.y), a3.z);

    vec4 fill = fragColor * fillColor;
    finalColor = mix(lineColor, fill, edgeFactor);
    
    if (finalColor.a < 0.01) {
        discard;
    }
}
