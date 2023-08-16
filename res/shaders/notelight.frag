#version 110

varying vec4 vFragPos;
uniform vec2 uSize;
uniform vec2 uCenter;
uniform vec4 uColor;

float grad(float f) {
    const float FAC = 0.65;
    return FAC * (1.0 - f);
}

float GAMMA = 2.2;

void main()
{
    vec4 pos = gl_ModelViewMatrix*vFragPos;
    float dstx = (uCenter.x - pos.x) / (uSize.x / 2.0);
    float dsty = (uCenter.y - pos.y) / (uSize.y / 2.0);
    float dst = (dstx * dstx + dsty * dsty);

    float gradient = max(0.0, grad(dst) - grad(1.0));
    gl_FragColor = vec4(uColor.rgb, pow(gradient*gradient, GAMMA));
}
