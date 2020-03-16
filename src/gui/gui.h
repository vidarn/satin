#pragma once
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_STANDARD_IO
#include <stdarg.h>
#include "nuklear.h"
#include "engine.h"

struct GuiContext
{
    struct nk_context ctx;
    struct GameData *data;
    
    int rect_shader;
    int circle_shader;
    int triangle_shader;
    int color_rect_shader;

	int font;
	struct InputState input;
	struct RenderCoord mp;
	struct RenderContext* render_context;
	float border_col[4];
	float bg_col[4];
	float button_col[4];
	float button_hover_col[4];
	float button_active_col[4];
	float text_col[4];
};

struct nk_user_font *gui_load_font(int font, struct GuiContext *context, struct GameData *data)
;
struct GuiContext *gui_init(int font, struct GameData *data);
void gui_begin_frame(struct GuiContext* gui, struct InputState input_state, struct RenderContext* render_context, struct GameData* data)
;
void gui_draw(struct GuiContext *gui, struct RenderContext *context);


void gui_fps_section(struct GuiContext *gui, struct GameData *data);
struct Color gui_color_picker(struct Color col, struct GuiContext *gui,
    struct GameData *data);

int gui_button(struct RenderRect button_rect, const char* caption, struct GuiContext* context)
;
