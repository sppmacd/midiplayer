#version 110

varying vec4 vColor;
uniform float uKeySize;
uniform bool uIsBlack;

void main()
{
    float offset = mod(gl_FragCoord.x / uKeySize, 1.0);
    float blackFactor = uIsBlack ? 0.4 : 0.8;
    float colorM = 0.7 * pow(2.0 * offset - 1.0, 2.0);
    if(offset > 0.05 && offset < 0.95)
        gl_FragColor = vec4(vColor.r * blackFactor + colorM, vColor.g * blackFactor + colorM, vColor.b * blackFactor + colorM, 1.0);
    else
        gl_FragColor = vec4(0,0,0,0);
}
