layout(location = 0) in vec2 pos;
uniform mat3 matrix;
void main()
{
    gl_Position = vec4( matrix * vec3(pos.x,pos.y,1.f), 1.f);
}
