#version 110

uniform sampler2D uInput;
uniform vec2 uInputSize;

vec4 sample(vec2 coords) {
    return texture2D(uInput, coords / uInputSize.xy);
}

vec4 calculate_blur() {
    vec4 result = vec4(0);
    const float BOX_BLUR_SIZE = 5.0;
    float dir = atan((gl_FragCoord.x - uInputSize.x / 2.0) / (gl_FragCoord.y - uInputSize.y));
    for (float x = 0.0; x < BOX_BLUR_SIZE; x++) {
        vec2 off = vec2(x*sin(dir), x*cos(dir));
        result += sample(gl_FragCoord.xy + off) * (1.0 - pow((x + 1.0) / BOX_BLUR_SIZE, 4.0));
    }
    float weight_sum = ((BOX_BLUR_SIZE + 1.0) * BOX_BLUR_SIZE / 2.0) / BOX_BLUR_SIZE;
    return result / weight_sum;
}

void main() {
    gl_FragColor = calculate_blur();
}
