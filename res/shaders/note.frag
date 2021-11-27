#version 120

varying vec4 vColor;
uniform vec2 uKeySize;
uniform vec2 uKeyPos;
uniform bool uIsBlack;

float offset_factor(float offset)
{
    return ((abs(0.5-offset)+0.5)-0.9) * 11.0;
}

void main()
{
    float offsetx = mod((gl_FragCoord.x - uKeyPos.x) / uKeySize.x, 1.0);
    float blackFactor = uIsBlack ? 0.4 : 0.8;
    float colorM = 0.6 * pow(2.0 * (offsetx) - 1.0, 2.0);
    if(offsetx > 0.1 && offsetx < 0.9)
        gl_FragColor = vec4(vColor.r * blackFactor + colorM, vColor.g * blackFactor + colorM, vColor.b * blackFactor + colorM, 1.0);
    else
        gl_FragColor = vec4(1,1,1,1-offset_factor(offsetx));
}
