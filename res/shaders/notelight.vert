#version 110

varying vec4 vColor;
varying vec4 vFragPos;

void main()
{
    vColor = gl_Color;
    vFragPos = gl_Vertex;
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
