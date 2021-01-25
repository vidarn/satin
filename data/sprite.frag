precision mediump float;
in vec2 uv;
out vec4 col;
uniform vec4 color;
uniform sampler2D sprite;
uniform vec4 sprite_uv;
void main()
{
    col = texture(sprite, uv*sprite_uv.zw + sprite_uv.xy);
}
