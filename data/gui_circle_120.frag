varying vec2 uv;
uniform vec4 color;
void main()
{
    float r = length(uv-vec2(0.5f));
    gl_FragColor = color*step(r,0.5f);
}
