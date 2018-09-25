#include "graphics/graphics.h"

struct FrameData *frame_data_new()
{
    struct FrameData *frame_data = calloc(1,sizeof(struct FrameData));
    return frame_data;
}

void frame_data_reset(struct FrameData *frame_data)
{
    struct RenderLineList *rll = &frame_data->render_line_list;
    rll->num = 0;
    while(rll->next != 0){
        rll = rll->next;
        rll->num = 0;
    }
    struct RenderSpriteList *rsl = &frame_data->render_sprite_list;
    rsl->num = 0;
    while(rsl->next != 0){
        rsl = rsl->next;
        rsl->num = 0;
    }
    struct RenderMeshList *rml = &frame_data->render_mesh_list;
    rml->num = 0;
    while(rml->next != 0){
        rml = rml->next;
        rml->num = 0;
    }
}

void set_active_frame_data(struct FrameData *frame_data, struct GameData *data)
{
    data->frame_data = frame_data;
}

struct FrameData *get_active_frame_data(struct GameData *data)
{
    return data->frame_data;
}


