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
        gl_FragColor = vec4(vColor.r+0.5, vColor.g+0.5, vColor.b+0.5, vColor.a);
    else
    {
        float gradient = sqrt(sqrt(dst) / (uRadius - kernRadius));
        gl_FragColor = vec4(vColor.r, vColor.g, vColor.b, max(0.0, 0.8-gradient)*vColor.a);
    }
}
