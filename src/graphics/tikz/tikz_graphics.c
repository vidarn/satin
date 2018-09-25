#include "../graphics.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void tikz_render_to_file(struct FrameData *frame_data,
    const char *output_filename, float w, float h)
{
    FILE *fp = fopen(output_filename, "wt");
    if(fp){
        fprintf(fp, "\\begin{tikzpicture}[x=%fcm, y=%fcm]\n",w,h);
        //fprintf(fp, "\\begin{scope}\n\\clip(-1,-1) rectangle(2,2)\n");
        //TODO(Vidar): clip lines outside boundary
        struct RenderLineList *render_line_list = &frame_data->render_line_list;
        while(render_line_list){
            int num_lines = render_line_list->num;
            for(int i_line=0;i_line<num_lines;i_line++){
                struct RenderLine *l = render_line_list->lines + i_line;
                if(fabsf(l->x1) < 100.f && fabsf(l->x2) < 100.f && fabsf(l->y1)
                    < 100.f && fabsf(l->y2) < 100.f)
                {
                    fprintf(fp, "\\definecolor{linecolor}{rgb}{%f,%f,%f}\n", l->r, l->g, l->b);
                    fprintf(fp, "\\draw [line width=%fcm, draw=linecolor] (%f,%f) -- (%f,%f);\n",
                        l->thickness*w*2.f, l->x1, l->y1, l->x2, l->y2);
                }
            }
            render_line_list = render_line_list->next;
        }
        struct RenderStringList *render_string_list = &frame_data->render_string_list;
        while(render_string_list){
            int num_strings = render_string_list->num;
            for(int i_string=0;i_string<num_strings;i_string++){
                struct RenderString *s = render_string_list->strings + i_string;
                fprintf(fp, "\\draw (%f,%f) node[anchor=south west]{%s};\n", s->x, s->y, s->str);
            }
            render_string_list = render_string_list->next;
        }
        //fprintf(fp, "\\end{scope}\n");
        fprintf(fp, "\\end{tikzpicture}\n");
        fclose(fp);
    }
}
