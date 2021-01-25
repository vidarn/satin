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


vertex float4 fill_vert(
    const device packed_float2* points [[ buffer(0) ]],
    unsigned int vid [[ vertex_id ]],
    constant packed_float3 *m [[buffer(1)]]
                        )
{
    float3x3 matrix(m[0],m[1],m[2]);
    return float4( matrix * float3(points[vid], 1.0), 1.0);
}


fragment half4 fill_frag(
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
    constant packed_float4 *view_m [[buffer(4)]],
    constant packed_float4 *proj_m [[buffer(5)]]
    )
{
    float4x4 model_matrix(model_m[0],model_m[1],model_m[2],model_m[3]);
    float4x4 view_matrix(view_m[0],view_m[1],view_m[2],view_m[3]);
    float4x4 proj_matrix(proj_m[0],proj_m[1],proj_m[2],proj_m[3]);
    struct MeshVertexData out;
    float3 p = pos[vid];
    out.pos  =  proj_matrix * view_matrix * model_matrix * float4(p, 1.0);
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

fragment half4 mesh_debug_frag(
    struct MeshVertexData in [[stage_in]]
    )
{
    return half4(float4(in.normal*.5f +float3(0.5f), 1.f));
}

struct SpriteMeshVertexData
{
    float4 pos[[position]];
    float2 uv;
    float light;
};

vertex struct SpriteMeshVertexData sprite_mesh_vert(
    const device packed_float2* pos [[ buffer(0) ]],
    const device packed_float2* uv [[ buffer(1) ]],
    const device float* light [[buffer(2)]],
    unsigned int vid [[ vertex_id ]],
    constant packed_float4 *model_m [[buffer(3)]],
    constant packed_float4 *view_m [[buffer(4)]]
    )
{
    float4x4 model_matrix(model_m[0],model_m[1],model_m[2],model_m[3]);
    float4x4 view_matrix(view_m[0],view_m[1],view_m[2],view_m[3]);
    struct SpriteMeshVertexData out;
    float2 p = pos[vid];
    out.pos  =  view_matrix * model_matrix * float4(p, 0.f, 1.0);
    out.light = light[vid];
    out.uv = uv[vid];
    return out;
}

fragment half4 sprite_mesh_frag(
    struct SpriteMeshVertexData in [[stage_in]],
    constant float4 &color [[buffer(5)]],
    constant float &light_amount[[buffer(6)]]
    )
{
    float light = 1.f + light_amount*(in.light-1.f);
    float4 lighting_col = float4(color.rgb*light,color.a);
    return (half4)lighting_col;
}



struct ShadowMeshVertexData
{
    float4 pos[[position]];
    float2 uv;
};

vertex struct ShadowMeshVertexData shadow_mesh_vert(
    const device packed_float2* pos [[ buffer(0) ]],
    const device packed_float2* uv [[ buffer(1) ]],
    unsigned int vid [[ vertex_id ]],
    constant packed_float4 *model_m [[buffer(2)]],
    constant packed_float4 *view_m [[buffer(3)]],
    constant float4 &sprite_uv [[buffer(4)]]
    )
{
    float4x4 model_matrix(model_m[0],model_m[1],model_m[2],model_m[3]);
    float4x4 view_matrix(view_m[0],view_m[1],view_m[2],view_m[3]);
    struct ShadowMeshVertexData out;
    float2 p = pos[vid];
    out.pos  =  view_matrix * model_matrix * float4(p, 1.f, 1.0);
    float2 this_uv = uv[vid];
    out.uv = vector_float2(sprite_uv[0] + sprite_uv[2]*this_uv[0], sprite_uv[1] + sprite_uv[3]*(1.f-p[1]));
    return out;
}

fragment half4 shadow_mesh_frag(
    struct ShadowMeshVertexData in [[stage_in]],
    texture2d<half> sprite [[ texture(5) ]]
    )
{
    //col = texture(sprite, uv_out*sprite_uv.zw + sprite_uv.xy);
    constexpr sampler s (mag_filter::linear, min_filter::linear);
    return sprite.sample(s,in.uv);
}


struct CharacterVertexData
{
    float4 pos[[position]];
    float2 uv;
    float2 back_light_uv;
};

vertex struct CharacterVertexData character_vert(
    const device packed_float2* points [[ buffer(0) ]],
    unsigned int vid [[ vertex_id ]],
    constant packed_float3 *m [[buffer(1)]],
    constant float4 &uv [[buffer(3)]],
    constant float4 &back_light_uv [[buffer(6)]]
    )
{
    float3x3 matrix(m[0],m[1],m[2]);
    struct CharacterVertexData out;
    float2 p = points[vid];
    out.pos  = float4( matrix * float3(p, 1.0), 1.0);
    out.uv = vector_float2(uv[0] + uv[2]*p[0], uv[1] + uv[3]*(1.f-p[1]));
    out.back_light_uv = vector_float2(back_light_uv[0] + back_light_uv[2]*p[0], back_light_uv[1] + back_light_uv[3]*(1.f-p[1]));
    return out;
}

fragment half4 character_frag(
    struct CharacterVertexData in [[stage_in]],
    constant float4 &color [[buffer(2)]],
    texture2d<half> sprite [[ texture(4) ]],
    constant float &light [[buffer(5)]],
    texture2d<half> back_light_sprite [[texture(7)]]
    )
{
    constexpr sampler s (mag_filter::linear, min_filter::linear);
    half4 tex = pow(sprite.sample(s,in.uv),half4(2.2));
    half4 back_tex = pow(sprite.sample(s, in.back_light_uv), half4(2.2));
    half3 l = (0.3*tex + 1.2*light*back_tex).rgb;
    return pow(half4(l,tex.a)*half4(color),half4(0.4545));
    //return (half4)uv*10.f;
}



struct TreeVertexData
{
    float4 pos[[position]];
    float2 val;
};

vertex struct TreeVertexData tree_vert(
    const device packed_float2* points [[ buffer(0) ]],
    unsigned int vid [[ vertex_id ]],
    constant packed_float3 *m [[buffer(1)]]
    )
{
    float3x3 matrix(m[0],m[1],m[2]);
    struct TreeVertexData out;
    float2 p = points[vid];
    out.pos  = float4( matrix * float3(p, 1.0), 1.0);
    out.val = p;
    return out;
}

fragment half4 tree_frag(
    struct TreeVertexData in [[stage_in]],
    constant float4 &color [[buffer(2)]],
    constant int *dimension[[buffer(3)]],
    constant float *split_value[[buffer(4)]],
    constant int *children[[buffer(5)]]
    )
{
    /*
    if(in.val > split_value[0]){
        return half4(in.val);
    }else{
        return half4(0.f);
    }
     */
    float val_min[] = {0.f, 0.f};
    float val_max[] = {1.f, 1.f};
    int node_index = 0;
    
    while(children[node_index*2] != 0){
        if(in.val[dimension[node_index]] < split_value[node_index]){
            val_max[dimension[node_index]] = split_value[node_index];
            node_index = children[node_index*2];
        }else{
            val_min[dimension[node_index]] = split_value[node_index];
            node_index = children[node_index*2+1];
        }
    }
 
    
    float v = 50.f * (val_max[0] - val_min[0]) * (val_max[1] - val_min[1]);
    return half4(v, 0, 0, 1);
    //return (half4)uv*10.f;
}

fragment half4 rf_mesh_frag(
    struct MeshVertexData in [[stage_in]],
    constant int *dimension[[buffer(5)]],
    constant float *split_value[[buffer(6)]],
    constant int *children[[buffer(7)]],
    constant int &num_nodes[[buffer(8)]]
    )
{
    float3 view_dir = float3(0.f,0.f,1.f);
    float3 light_dir = normalize(float3(1.f, 0.3f, 1.f));
    float3 half_dir = normalize(view_dir + light_dir);
    float d = dot(half_dir, normalize(in.normal));
    float alpha = 3.f;
    
    if(in.pos.x > 800.f){
        float val[] = {d,alpha/150.f};
        float val_min[] = {0.f, 0.f};
        float val_max[] = {1.f, 1.f};
        int node_index = 0;
        /*
        while(children[node_index*2] != 0){
            if(val < split_value[node_index]){
                val_max[dimension[node_index]] = split_value[node_index];
                node_index = children[node_index*2];
            }else{
                val_min[dimension[node_index]] = split_value[node_index];
                node_index = children[node_index*2+1];
            }
        }
        float l = 0.001f * (float)node_index;//(val_max[0] - val_min[0]);
        */

        while(children[node_index*2] != 0){
            if(val[dimension[node_index]] < split_value[node_index]){
                val_max[dimension[node_index]] = split_value[node_index];
                node_index = children[node_index*2];
            }else{
                val_min[dimension[node_index]] = split_value[node_index];
                node_index = children[node_index*2+1];
            }
        }
        float l = 0.1f/((val_max[0] - val_min[0])*(val_max[1] - val_min[1])*(float)num_nodes);
        return half4(l, l, l, 1.f);
    }else{
        float l = pow(d,alpha)* (alpha+8.f)/(8.f*M_PI_F);
        return half4(float4(l, l, l, 1.f));
    }
    
}
