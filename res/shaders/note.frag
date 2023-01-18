#version 120

varying vec4 vColor;
uniform vec2 uKeySize;
uniform vec2 uKeyPos;
uniform bool uIsBlack;

const float BorderRadius = 8.0;
const float BloomRadius = 10.0;
const float AntialiasRadius = 1.0;

float borderRadius() {
    return min(min(BorderRadius, uKeySize.x / 2.0), uKeySize.y / 2.0);
}

bool isCorner()
{
    float br = borderRadius();
    bool is_x_corner = gl_FragCoord.x < uKeyPos.x + br || gl_FragCoord.x > uKeyPos.x + uKeySize.x - br;
    bool is_y_corner = gl_FragCoord.y < uKeyPos.y + br || gl_FragCoord.y > uKeyPos.y + uKeySize.y - br;
    return is_x_corner && is_y_corner;
}

float distanceFromEdge() {
    float br = borderRadius();
    bool is_left = gl_FragCoord.x < uKeyPos.x + br;
    bool is_top = gl_FragCoord.y < uKeyPos.y + br;
    bool is_right = gl_FragCoord.x > uKeyPos.x + uKeySize.x - br;
    bool is_bottom = gl_FragCoord.y > uKeyPos.y + uKeySize.y - br;
    float x = is_left ? gl_FragCoord.x - (uKeyPos.x + br) : (is_right ? gl_FragCoord.x - (uKeyPos.x + uKeySize.x - br) : 0);
    float y = is_top ? gl_FragCoord.y - (uKeyPos.y + br) : (is_bottom ? gl_FragCoord.y - (uKeyPos.y + uKeySize.y - br) : 0);
    if (x == 0 && y == 0) {
        return -1.0;
    }
    return sqrt(x*x+y*y);
}

float lightness(vec4 color) {
    return max(color.r, max(color.g, color.b));
}

vec4 illumination(float r) {
    float br = borderRadius();
    float sideFactor = (1 - (gl_FragCoord.x - uKeyPos.x) / uKeySize.x);
    float illumination = lightness(vColor) * 0.25 * sideFactor;
    float rScaled = pow(r / br, 2);
    return vColor + vec4(1,1,1,0) * (rScaled*illumination);
}

vec4 bloom(float r) {
    float br = borderRadius();
    const float BloomStart = 0.4;
    float rScaled = (r - br) / BloomRadius;
    float blurFactor = BloomStart - rScaled * BloomStart;
    return vec4(vColor.rgb, vColor.a * blurFactor);
}

void main()
{
    float br = borderRadius();
    float r = distanceFromEdge();
    if (r < 0) {
        // Full color
        gl_FragColor = vColor;
    }
    else if (r < br) {
        // Illumination
        gl_FragColor = illumination(r);
    }
    else if (r < br + AntialiasRadius){
        // Antialias
        vec4 i = illumination(r);
        vec4 b = bloom(r);
        float fac = (r - br) / AntialiasRadius;
        gl_FragColor = i * (1 - fac) + b * fac;
    }
    else {
        // Bloom
        gl_FragColor = bloom(r);
    }
}
