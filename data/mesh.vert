in vec3 pos;
in vec3 normal;
in vec3 uv_map;
uniform mat4 model_matrix;
uniform mat4 view_matrix;
out vec3 normal_out;
out vec3 uv_map_out;
out vec3 pos_out;
out vec3 view_out;
void main()
{
    vec4 pos = model_matrix * vec4(pos,1.f);
    pos_out = pos.xyz;
	gl_Position = view_matrix * pos;

	//TODO(Vidar):Don't invert the matrix in the shader!! Crazy...
    vec4 eye_pos = inverse(view_matrix) * vec4(0.f, 0.f, 0.f ,1.f);
	view_out = normalize(eye_pos.xyz - gl_Position.xyz);
	//view_out = vec3(0.f,0.f,1.f);

	//NOTE(Vidar): Assume that the model matrix is orthogonal
    normal_out = (model_matrix * vec4(normal,0.f)).xyz;
    //normal_out = normalize(transpose(inverse(view_matrix))*(model_matrix * vec4(normal,0.f))).xyz;
    uv_map_out = uv_map;
}
