#version 110

uniform vec4 uColorOverlay;
uniform vec4 uColorBackground;

float ditherMix(float a, float b, float fac) {
    // scale fac to be between a and b
    float scaledA = a * (1.0 - fac) + b * fac;

    // clamp blend factor
    scaledA = clamp(scaledA, 0.0, 1.0);

    // simple per-pixel hash -> random in [0,1)
    float rnd = fract(sin(dot(gl_FragCoord.xy / 100.0, vec2(12.9898,78.233))) * 43758.5453);

    // stochastic dithering using 256 levels (so 0.5 -> 50/50)
    float outA = floor(scaledA * 256.0 + rnd + 1.5) / 256.0;
    
    return outA;
}

void main()
{
    float a = -(gl_ModelViewProjectionMatrix * gl_FragCoord).y;

    // Blend between background and overlay based on y coordinate with dithering
    float r = ditherMix(uColorBackground.r, uColorOverlay.r, a);
    float g = ditherMix(uColorBackground.g, uColorOverlay.g, a);
    float b = ditherMix(uColorBackground.b, uColorOverlay.b, a);
    float alpha = ditherMix(uColorBackground.a, uColorOverlay.a, a);
    vec4 color = vec4(r, g, b, alpha);
    gl_FragColor = color;
}
