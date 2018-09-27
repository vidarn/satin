precision mediump float;
in vec3 normal_out;
in vec3 uv_map_out;
out vec4 col;

const int num_lights = 4;
uniform vec4 light_col[num_lights];
uniform vec3 light_dir[num_lights];
void main()
{
    //vec3 albedo = modf(uv_map_out,blaj);
    vec3 albedo = vec3(0.9f);
    vec3 accum_col = vec3(0.f);
    for(int i=1;i<num_lights;i++){
        float val = clamp(dot(normal_out,light_dir[i]),0.f,1.f);
        accum_col += light_col[i].rgb*val;
    }
    col = vec4(accum_col*albedo,1.0);
}
