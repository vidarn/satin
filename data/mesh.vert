layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 uv_map;
layout(location = 3) in vec4 tangent;
uniform mat4 model_matrix;
uniform mat4 view_matrix;
uniform mat4 projection_matrix;
out mat3 normal_matrix_out;
out vec3 uv_map_out;
out vec3 pos_out;
void main()
{
    vec4 pos = model_matrix * vec4(pos,1.f);
    pos_out = pos.xyz;
	gl_Position = projection_matrix * view_matrix * pos;

    vec3 n = normalize((model_matrix * vec4(normal,0.f)).xyz);
    vec3 t = normalize((model_matrix * vec4(tangent.xyz,0.f)).xyz);
	vec3 b = tangent.w*normalize(cross(n,t));
	normal_matrix_out = mat3(t,b,n);
    uv_map_out = uv_map;
}
