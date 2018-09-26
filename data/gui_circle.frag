out vec4 col;
in vec2 uv;
uniform vec4 color;
void main()
{
    float r = length(uv-vec2(0.5f));
    col = color*step(r,0.5f);
}
