#version 110

varying vec4 vFragPos;
uniform vec2 uSize;
uniform vec2 uCenter;
uniform vec4 uColor;

float grad(float f) {
    const float FAC = 0.7;
    return FAC * (1.0 - f);
}

float GAMMA = 2.2;

vec3 gamma_to_linear(vec3 c) {
    // return c;
    return vec3(pow(c.r, 1.0/GAMMA), pow(c.g, 1.0/GAMMA), pow(c.b, 1.0/GAMMA));
}

vec3 linear_to_gamma(vec3 c) {
    // return c;
    return vec3(pow(c.r, GAMMA), pow(c.g, GAMMA), pow(c.b, GAMMA));
}

void main()
{
    vec4 pos = gl_ModelViewMatrix*vFragPos;
    float dstx = (uCenter.x - pos.x) / (uSize.x / 2.0);
    float dsty = (uCenter.y - pos.y) / (uSize.y / 2.0);
    float dst = (dstx * dstx + dsty * dsty);

    float gradient = grad(dst) - grad(1.0);
    gl_FragColor = vec4(linear_to_gamma(gamma_to_linear(uColor.rgb) * gradient), gradient);
}
