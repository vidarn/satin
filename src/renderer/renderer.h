#pragma once
#include "compiler_features/compiler_features.h"
struct FrameData;

struct Sprite;

struct RenderSprite{
    struct Sprite *sprite;
    float pos[2];
    float width;
    float override_width;
    int render_pass;
};

struct RenderString{
    char *str;
    int font;
    float x, y;
};

struct RenderMesh{
    struct GraphicsValueSpec *uniforms;
	void (*callback)(void *param);
	void *callback_param;
    float m[16];
    float cam[16];
    int mesh;
    int shader;
    int num_uniforms;
	int depth_test;
};

struct RenderLine{
    float x1, y1, x2, y2;
    float thickness;
    float r,g,b;
    int render_pass;
};

struct RenderQuad{
    struct GraphicsValueSpec *uniforms;
	//NOTE(Vidar):This is a matrix3
	//TODO(Vidar):Handle this better
    float ALIGNED_(32) m[9];
	float pad[7];
    int num_uniforms;
    int shader;
};

#define render_sprite_list_size 2048
struct RenderStringList{
    int num;
    struct RenderString strings[render_sprite_list_size];
    struct RenderStringList *next;
};

struct RenderSpriteList{
    int num;
    struct RenderSprite sprites[render_sprite_list_size];
    struct RenderSpriteList *next;
};

struct RenderLineList{
    int num;
    struct RenderLine lines[render_sprite_list_size];
    struct RenderLineList *next;
};
struct RenderMeshList{
    int num;
    struct RenderMesh meshes[render_sprite_list_size];
    struct RenderMeshList *next;
};
struct RenderQuadList{
    int num;
    struct RenderQuad quads[render_sprite_list_size];
    struct RenderQuadList *next;
};

struct FrameData{
    struct RenderSpriteList render_sprite_list;
    struct RenderStringList render_string_list;
    struct RenderLineList   render_line_list;
    struct RenderMeshList   render_mesh_list;
    struct RenderQuadList   render_quad_list;
    int clear;
    struct Color clear_color;
};
