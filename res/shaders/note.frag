#version 120

varying vec4 vColor;
uniform vec2 uKeySize;
uniform vec2 uKeyPos;
uniform bool uIsBlack;

void main()
{
    float offsetx = mod((gl_FragCoord.x - uKeyPos.x) / uKeySize.x, 1.0);
    float offsety = (gl_FragCoord.y - uKeyPos.y) / uKeySize.y;
    float blackFactor = uIsBlack ? 0.4 : 0.8;
    float colorM = 0.7 * pow(2.0 * (offsetx) - 1.0, 2.0);
    if(offsetx > 0.05 && offsetx < 0.95)
        gl_FragColor = vec4(vColor.r * blackFactor + colorM, vColor.g * blackFactor + colorM, vColor.b * blackFactor + colorM, 1.0);
    else
        gl_FragColor = vec4(0,1,1,0);
}
