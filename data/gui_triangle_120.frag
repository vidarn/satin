varying vec2 uv;
uniform vec3 color;
uniform vec2 a;
uniform vec2 b;
uniform vec2 c;
void main()
{
    vec2 v0 = uv.xy-a;
    vec2 v1 = b-a;
    vec2 v2 = c-a;
    
    // Compute dot products
    float dot00 = dot(v0, v0);
    float dot01 = dot(v0, v1);
    float dot02 = dot(v0, v2);
    float dot11 = dot(v1, v1);
    float dot12 = dot(v1, v2);
    
    // Compute barycentric coordinates
    float invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
    float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v = (dot00 * dot12 - dot01 * dot02) * invDenom;
    
    float a1 = step(u,0.f);
    float a2 = step(v,0.f);
    float a3 = step(1.f-u-v,0.f);
    float a_tot = a1*a2*a3;
    /*
    // Check if point is in triangle
    return (u >= 0) && (v >= 0) && (u + v < 1)
    
    */
    a_tot = a3*a2;
    gl_FragColor = vec4(color*a_tot,a_tot);
}
