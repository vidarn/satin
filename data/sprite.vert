layout(location = 0) in vec2 pos;
uniform mat3 matrix;
out vec2 uv;
void main()
{
	gl_Position = vec4( matrix * vec3(pos.x,pos.y,1.f), 1.f);
	uv = vec2(pos.x,1.f-pos.y);
}
