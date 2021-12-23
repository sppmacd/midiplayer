#version 110

void main()
{
    gl_FragColor = vec4(0, 0, 0, -(gl_ModelViewProjectionMatrix * gl_FragCoord).y);
}
