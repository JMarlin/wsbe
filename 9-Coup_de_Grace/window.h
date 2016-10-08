#ifndef WINDOW_H
#define WINDOW_H 

#include "context.h"
#include <inttypes.h>

//================| Window Class Declaration |================//

//Feel free to play with this 'theme'
#define WIN_BGCOLOR     0xFFBBBBBB //A generic grey
#define WIN_TITLECOLOR  0xFFD09070 //A nice subtle blue
#define WIN_TITLECOLOR_INACTIVE 0xFF908080 //A darker shade 
#define WIN_BORDERCOLOR 0xFF000000 //Straight-up black
#define WIN_TITLEHEIGHT 31 
#define WIN_BORDERWIDTH 3

//Some flags to define our window behavior
#define WIN_NODECORATION 0x1

//Forward struct declaration for function type declarations
struct Window_struct;

//Callback function type declarations
typedef void (*WindowPaintHandler)(struct Window_struct*);
typedef void (*WindowMousedownHandler)(struct Window_struct*, int, int);

typedef struct Window_struct {  
    struct Window_struct* parent;
    int16_t x;
    int16_t y;
    uint16_t width;
    uint16_t height;
    uint16_t flags;
    Context* context;
    struct Window_struct* drag_child;
    struct Window_struct* active_child;
    List* children;
    uint16_t drag_off_x;
    uint16_t drag_off_y;
    uint8_t last_button_state;
    WindowPaintHandler paint_function;
    WindowMousedownHandler mousedown_function;
} Window;

//Methods
Window* Window_new(int16_t x, int16_t y, uint16_t width,
                   uint16_t height, uint16_t flags, Context* context);
int Window_init(Window* window, int16_t x, int16_t y, uint16_t width,
                uint16_t height, uint16_t flags, Context* context);
int Window_screen_x(Window* window);
int Window_screen_y(Window* window);                   
void Window_paint(Window* window, List* dirty_regions, uint8_t paint_children);
void Window_process_mouse(Window* window, uint16_t mouse_x,
                          uint16_t mouse_y, uint8_t mouse_buttons);
void Window_paint_handler(Window* window);
void Window_mousedown_handler(Window* window, int x, int y);
List* Window_get_windows_above(Window* parent, Window* child);
List* Window_get_windows_below(Window* parent, Window* child);
void Window_raise(Window* window, uint8_t do_draw);
void Window_move(Window* window, int new_x, int new_y);
Window* Window_create_window(Window* window, int16_t x, int16_t y,  
                             uint16_t width, int16_t height, uint16_t flags);
void Window_insert_child(Window* window, Window* child);   
void Window_invalidate(Window* window, int top, int left, int bottom, int right);                         

#endif //WINDOW_H
