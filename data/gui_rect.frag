out vec4 col;
in vec2 uv;
uniform vec3 color;
uniform vec2 radius;
uniform int outline;
uniform vec2 thickness;

float chamfer_box(vec2 xy, vec2 radius, vec2 thickness)
{
    vec2 end = vec2(1.f) - thickness*2.f;
    xy = smoothstep(end-radius*2, end, xy);
    return step(length(xy), 1.f);
}

void main()
{
    vec2 xy = 2.f*abs(uv - vec2(0.5f));
    float a = chamfer_box(xy, radius, vec2(0.f));
    if(outline == 1){
        float a2 = chamfer_box(xy, radius - thickness, thickness);
        vec2 a3 = step(thickness*2.f, vec2(1.f) - xy);
        a2 *= a3.x;
        a2 *= a3.y;
        float a4 = (1.f-a2)*a;
        col = vec4(color*a4, a4);
    }else{
        col = vec4(color*a, a);
    }
}
