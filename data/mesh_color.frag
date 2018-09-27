precision mediump float;
in vec3 normal_out;
in vec3 uv_map_out;
out vec4 col;

const int num_lights = 4;
uniform vec3 color;
void main()
{
    col = vec4(color,1.0);
}
