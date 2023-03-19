#version 110

varying vec4 vColor;
varying vec4 vFragPos;
uniform vec2 uCenter;
uniform float uRadius;
uniform float uGlowSize;

void main()
{
    float kernRadius = uRadius * uGlowSize;
    vec4 pos = gl_ModelViewMatrix*vFragPos;
    float dstx = uCenter.x-pos.x;
    float dsty = uCenter.y-pos.y;
    float dst = (dstx*dstx+dsty*dsty);
    if(dst < kernRadius*kernRadius)
        gl_FragColor = vec4(1, 1, 1, vColor.a);
    else
    {
        float gradient = pow(sqrt(dst) / (uRadius - kernRadius), 0.5);
        gl_FragColor = vec4(1, 1, 1, max(0.0, 0.7-gradient)*vColor.a);
    }
}
