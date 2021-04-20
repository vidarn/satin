precision mediump float;
in mat3 normal_matrix_out;
in vec3 uv_map_out;
in vec3 pos_out;
out vec4 col;

const int num_lights = 3;
uniform vec4 light_col[num_lights];
uniform vec3 light_dir[num_lights];
uniform vec3 color;
void main()
{
	vec3 normal_t = vec3(0.f,0.f,1.f);
    vec3 normal = normalize(normal_matrix_out * normal_t);
    vec3 albedo = color;
    vec3 accum_col = vec3(0.f);
    for(int i=0;i<num_lights;i++){
        float val = clamp(dot(normal,normalize(light_dir[i])),0.f,1.f);
        accum_col += light_col[i].rgb*val;
    }
    col = vec4(accum_col*albedo,1.0);
}
