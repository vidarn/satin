in vec3 pos;
in vec3 normal;
in vec3 uv_map;
uniform mat4 model_matrix;
uniform mat4 view_matrix;
out vec3 normal_out;
out vec3 uv_map_out;
out vec3 pos_out;
void main()
{

    vec4 pos = model_matrix * vec4(pos,1.f);
    pos_out = pos.xyz;
	gl_Position = view_matrix * pos;
    normal_out = normal;
    uv_map_out = uv_map;
}