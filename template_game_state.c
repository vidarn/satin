#include "engine.h"
#include <stdlib.h>

struct TemplateStateData{
    struct RenderContext *context;
    struct FrameData *main_frame_data;
};
static struct TemplateStateData *state_data = 0;
static struct RenderContext *context = 0;

void init_template(struct GameData *data, void *argument, int parent_state)
{
    state_data = calloc(1,sizeof(struct TemplateStateData));
    set_custom_data_pointer(state_data,data);
    state_data->context = calloc(1,sizeof(struct RenderContext));
    state_data->main_frame_data = frame_data_new();
    state_data->context->frame_data = state_data->main_frame_data;
}

void destroy_template(struct GameData *data)
{
    free(state_data->context);
    free(state_data->main_frame_data);
    free(state_data);
}

int update_template(int ticks, struct InputState input_state,
                 struct GameData *data)
{
    state_data = get_custom_data_pointer(data);
    context = state_data->context;
    context->w = context->h = 1.0f;
    context->data = data;

    set_active_frame_data(state_data->main_frame_data,data);
    frame_data_reset(state_data->main_frame_data);
    struct Color clear_col = {0.8f,0.8f,0.8f,0.f};
    frame_data_clear(state_data->main_frame_data, clear_col);

	context->camera_2d = get_identity_matrix3();
	context->camera_3d = get_identity_matrix4();

	return 0;//NOTE(Vidar): Return 1 to wait for input before calling update again (e.g. from a GUI)
}

struct GameState template_game_state = {
	update_template,
	init_template,
	destroy_template,
	0
};
