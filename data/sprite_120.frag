varying vec2 uv;
uniform vec4 color;
uniform sampler2D sprite;
uniform vec4 sprite_uv;
void main()
{
    vec4 c = texture2D(sprite, uv*sprite_uv.zw + sprite_uv.xy)*color;
    gl_FragColor = c;
}
