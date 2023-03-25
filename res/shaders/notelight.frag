#version 110

varying vec4 vFragPos;
uniform vec2 uSize;
uniform vec2 uCenter;
uniform vec4 uColor;

float grad(float f) {
    const float FAC = 15.0;
    const float SCALE = 1.001;
    const float EXP = 2.0;
    return pow(EXP, FAC * (SCALE - f) - FAC);
}

struct HSL {
    float h; //0..360
    float s; //0..1
    float l; //0..1
};

HSL rgb_to_hsl(vec3 rgb) {
    float cmax = max(max(rgb.r, rgb.g), rgb.b);
    float cmin = min(min(rgb.r, rgb.g), rgb.b);
    float delta = cmax - cmin;
    HSL hsl;
    if (delta == 0.0) {
        hsl.h = 0.0;
    } else if (cmax == rgb.r) {
        hsl.h = 60.0 * mod((rgb.g - rgb.b)/delta, 6.0);
    } else if (cmax == rgb.g) {
        hsl.h = 60.0 * ((rgb.b - rgb.r)/delta + 2.0);
    } else {
        hsl.h = 60.0 * ((rgb.r - rgb.g)/delta + 4.0);
    }
    hsl.l = (cmax - cmin) / 2.0;
    hsl.s = delta == 0.0 ? 0.0 : delta / (1.0 - abs(2.0 * hsl.l - 1.0));
    return hsl;
}

vec3 hsl_to_rgb(HSL hsl) {
    float c = (1.0 - abs(2.0 * hsl.l - 1.0)) * hsl.s;
    float x = c * (1.0 - abs(mod(hsl.h / 60.0, 2.0) - 1.0));
    float m = hsl.l - c / 2.0;
    vec3 color;
    if (hsl.h >= 0.0 && hsl.h < 60.0) {
        color = vec3(c,x,0.0);
    }
    else if (hsl.h >= 60.0 && hsl.h < 120.0) {
        color = vec3(x,c,0.0);
    }
    else if (hsl.h >= 120.0 && hsl.h < 180.0) {
        color = vec3(0.0,c,x);
    }
    else if (hsl.h >= 180.0 && hsl.h < 240.0) {
        color = vec3(0.0,x,c);
    }
    else if (hsl.h >= 240.0 && hsl.h < 300.0) {
        color = vec3(x,0.0,c);
    }
    else {
        color = vec3(c,0.0,x);
    }
    return color + vec3(m, m, m);
}

void main()
{
    vec4 pos = gl_ModelViewMatrix*vFragPos;
    float dstx = (uCenter.x - pos.x) / (uSize.x / 2.0);
    float dsty = (uCenter.y - pos.y) / (uSize.y / 2.0);
    float dst = (dstx * dstx + dsty * dsty);

    float gradient = grad(dst) - grad(1.0);

    HSL color_hsl = rgb_to_hsl(uColor.rgb);
    color_hsl.l = min(1.0, max(0.0, gradient));
    gl_FragColor = vec4(hsl_to_rgb(color_hsl), 1.0);
}
