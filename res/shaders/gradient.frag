#version 110

uniform vec4 uColor;

void main()
{
    gl_FragColor = vec4(1, 1, 1, -(gl_ModelViewProjectionMatrix * gl_FragCoord).y) * uColor;
}
