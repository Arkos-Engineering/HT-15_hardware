#include "defines.h"

#if !defined(OSR_UI)
#define OSR_UI

#if !defined(OSR_UI_EXPORT)
#define OSR_UI_EXPORT
#endif

typedef enum PACKED{
        osrui_text_type,
        osrui_image_type,
        osrui_button_type,
        osrui_end_button_type,
        osrui_begin_list_vertical_type,
        osrui_end_list_vertical_type,
        osrui_begin_list_horizontal_type,
        osrui_end_list_horizontal_type,
} osrui_command_type;

typedef struct {
        c_str text;
        enum PACKED options{
                osrui_editable = 1 <<0,
                //if 0 fades else wraps text.
                osrui_fade_or_wrap = 1 <<0,

        } options;
} osrui_text_command;
typedef struct {

} osrui_image_command;
typedef struct {

} osrui_button_command;
typedef struct {

} osrui_list_vertical_command;
typedef struct {

} osrui_end_list_vertical_command;
typedef struct {

} osrui_list_horizontal_command;
typedef struct {

} osrui_end_list_horizontal_command;

typedef struct{
        osrui_command_type type;
        u8 id;
} osrui_component_id;

typedef struct{
        //Inexes into component ids.
        u16 component_index;
        osrui_command_type type;
} osrui_component;

typedef enum{
        osrui_selected = 1,
        osrui_pressed,
} osrui_component_state;

#define MAX_DRAW_COMMANDS 200
typedef struct{
        u16 width,height;
        void * window_buffer;
        //TODO as of Mar 11 2025: hold onto previous state for each component to tell if it needs to be re-rendered for its section of the screen. @Zea Lynn

        //TODO as of Mar 11 2025: component_ids can grow larger than MAX_DRAW_COMMANDS due to it being the way to address unique components. @Zea Lynn
        osrui_component_id            component_ids[MAX_DRAW_COMMANDS];
        osrui_component               components[MAX_DRAW_COMMANDS];
        osrui_text_command            text_commands[MAX_DRAW_COMMANDS];
        osrui_image_command           image_commands[MAX_DRAW_COMMANDS];
        osrui_button_command          button_commands[MAX_DRAW_COMMANDS];
        osrui_list_vertical_command   list_vertical_commands[MAX_DRAW_COMMANDS];
        osrui_list_horizontal_command list_horizontal_commands[MAX_DRAW_COMMANDS];

        u16                           component_id_count;
        u8                            commands_count;
} osrui_state;

typedef struct{
        u8 pp_0:2;
        u8 pp_1:2;
        u8 pp_2:2;
        u8 pp_3:2;
} osrui_paper_pixels;
OSR_UI_EXPORT bool osrui_begin(osrui_state * state);
//TODO as of March 11 2025: figure out how to only tell the paper display controller what parts of the buffer to re_render. @Zea Lynn
OSR_UI_EXPORT bool osrui_end_and_render(osrui_state * state, void * output_buffer);

//mest be acompanied with end button
OSR_UI_EXPORT osrui_component_state osrui_button(osrui_state * state, u16 component_id, osrui_button_command command);
//returns false if there was a problem ending the button.
OSR_UI_EXPORT bool osrui_end_button(osrui_state * state);

#if !defined(OSR_UI_IMPLEMENTATION) && !defined(OSR_UI_OMIT_IMPLEMENTATION)
#define OSR_UI_IMPLEMENTATION

#endif /* OSR_UI_IMPLEMENTATION */
#endif /* OSR_UI */
