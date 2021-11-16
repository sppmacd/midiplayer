#version 110

varying vec4 vColor;
varying vec4 vFragPos;
uniform vec2 uCenter;
uniform float uRadius;

void main()
{
    float sRadius = uRadius * 0.25;
    vec4 pos = gl_ModelViewMatrix*vFragPos;
    float dstx = uCenter.x-pos.x;
    float dsty = uCenter.y-pos.y;
    float dst = (dstx*dstx+dsty*dsty);
    if(dst < sRadius*sRadius)
        gl_FragColor = vec4(vColor.r+0.5, vColor.g+0.5, vColor.b+0.5, vColor.a);
    else
    {
        float gradient = sqrt(sqrt(dst) / (uRadius - sRadius));
        gl_FragColor = vec4(vColor.r, vColor.g, vColor.b, max(0.0, 1.0-gradient)*vColor.a);
    }
}
