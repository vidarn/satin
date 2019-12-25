//
//  Shaders.metal
//  metal_test
//
//  Created by Vidar Nelson on 2019-10-18.
//  Copyright © 2019 Vidar Nelson. All rights reserved.
//

#include <metal_stdlib>
using namespace metal;


vertex float4 basic_vertex(                           // 1
                           const device packed_float3* vertex_array [[ buffer(0) ]], // 2
                           unsigned int vid [[ vertex_id ]]) {                 // 3
    return float4(vertex_array[vid], 1.0);              // 4
}


fragment half4 basic_fragment() { // 1
    return half4(1.0);              // 2
}


vertex float4 line_vert(
    const device packed_float2* points [[ buffer(0) ]],
    unsigned int vid [[ vertex_id ]],
    constant packed_float3 *m [[buffer(1)]]
                        )
{
       // gl_Position = vec4( matrix * vec3(pos.x,pos.y,1.f), 1.f);
    float3x3 matrix(m[0],m[1],m[2]);
    return float4( matrix * float3(points[vid], 1.0), 1.0);
}


fragment half4 line_frag(
    constant float4 &color [[buffer(2)]]
    )
{
    return (half4)color;
}

struct SpriteVertexData
{
    float4 pos[[position]];
    float2 uv;
};

vertex struct SpriteVertexData sprite_vert(
    const device packed_float2* points [[ buffer(0) ]],
    unsigned int vid [[ vertex_id ]],
    constant packed_float3 *m [[buffer(1)]],
    constant float4 &uv [[buffer(3)]]
    )
{
    float3x3 matrix(m[0],m[1],m[2]);
    struct SpriteVertexData out;
    float2 p = points[vid];
    out.pos  = float4( matrix * float3(p, 1.0), 1.0);
    out.uv = vector_float2(uv[0] + uv[2]*p[0], uv[1] + uv[3]*(1.f-p[1]));
    return out;
}


fragment half4 sprite_frag(
    struct SpriteVertexData in [[stage_in]],
    constant float4 &color [[buffer(2)]],
    texture2d<half> sprite [[ texture(4) ]]
    )
{
    constexpr sampler s (mag_filter::linear, min_filter::linear);
    return sprite.sample(s, in.uv) * half4(color);
    //return (half4)uv*10.f;
}



struct MeshVertexData
{
    float4 pos[[position]];
    float3 normal;
};

vertex struct MeshVertexData mesh_vert(
    const device packed_float3* pos [[ buffer(0) ]],
    const device packed_float3* normal [[ buffer(1) ]],
    const device packed_float3* uv [[ buffer(2) ]],
    unsigned int vid [[ vertex_id ]],
    constant packed_float4 *model_m [[buffer(3)]],
    constant packed_float4 *view_m [[buffer(4)]]
    )
{
    float4x4 model_matrix(model_m[0],model_m[1],model_m[2],model_m[3]);
    float4x4 view_matrix(view_m[0],view_m[1],view_m[2],view_m[3]);
    struct MeshVertexData out;
    float3 p = pos[vid];
    out.pos  =  view_matrix * model_matrix * float4(p, 1.0);
    //NOTE(Vidar):Assume that the model matrix is orthogonal (no non-uniform scaling)
    out.normal = float3(model_matrix * float4(normal[vid], 0.f));
    return out;
}


fragment half4 mesh_color_frag(
    struct MeshVertexData in [[stage_in]],
    constant float4 &color [[buffer(5)]]
    )
{
    return half4(color);
}

fragment half4 mesh_frag(
    struct MeshVertexData in [[stage_in]],
    constant float4 &color [[buffer(5)]]
    )
{
    return half4(float4(in.normal, 1.f));
}
