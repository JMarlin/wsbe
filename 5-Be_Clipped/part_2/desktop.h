#ifndef DESKTOP_H
#define DESKTOP_H

#include "list.h"
#include "context.h"
#include "window.h"


//================| Desktop Class Declaration |================//

typedef struct Desktop_struct {
    List* children;
    Context* context;
    uint8_t last_button_state;
    uint16_t mouse_x;
    uint16_t mouse_y;
    Window* drag_child;
    uint16_t drag_off_x;
    uint16_t drag_off_y;
} Desktop;

//Methods
Desktop* Desktop_new(Context* context);
Window* Desktop_create_window(Desktop* desktop, unsigned int x, unsigned int y,  
                              unsigned int width, unsigned int height);
void Desktop_process_mouse(Desktop* desktop, uint16_t mouse_x,
                      uint16_t mouse_y, uint8_t mouse_buttons);
void Desktop_paint(Desktop* desktop);
List* Desktop_get_windows_above(Desktop* desktop, Window* window);

#endif //DESKTOP_H
