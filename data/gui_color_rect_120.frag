varying vec2 uv;
uniform vec4 colors[4];

const float gamma = 1.f;

vec4 toLinear(vec4 v) {
    return pow(v, vec4(gamma));
}

vec4 toGamma(vec4 v) {
    return pow(v, vec4(1.0 / gamma));
}

void main()
{
    vec4 col_bottom = toLinear(colors[0])*(1.f-uv.x) + toLinear(colors[1])*uv.x;
    vec4 col_top = toLinear(colors[2])*(1.f-uv.x) + toLinear(colors[3])*uv.x;
    gl_FragColor = toGamma(col_bottom*(1.f-uv.y) + col_top*uv.y);
}
