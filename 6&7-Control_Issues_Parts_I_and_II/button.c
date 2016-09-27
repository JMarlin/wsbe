#include "button.h"

Button* Button_new(int x, int y, int w, int h) {

    //Normal allocation and initialization
    //Like a Desktop, this is just a special kind of window 
    Button* button;
    if(!(button = (Button*)malloc(sizeof(Button))))
        return button;

    if(!Window_init((Window*)button, x, y, w, h, WIN_NODECORATION, (Context*)0)) {

        free(button);
        return (Button*)0;
    }

    //Override default window callbacks
    button->window.paint_function = Button_paint;
    button->window.mousedown_function = Button_mousedown_handler;

    //And clear the toggle value
    button->color_toggle = 0;

    return button;
}

void Button_paint(Window* button_window) {

    Button* button = (Button*)button_window;

    uint32_t border_color;
    if(button->color_toggle)
        border_color = WIN_TITLECOLOR;
    else
        border_color = WIN_BGCOLOR - 0x101010;

    Context_fill_rect(button_window->context, 1, 1, button_window->width - 1,
                      button_window->height - 1, WIN_BGCOLOR);
    Context_draw_rect(button_window->context, 0, 0, button_window->width,
                      button_window->height, 0xFF000000);
    Context_draw_rect(button_window->context, 3, 3, button_window->width - 6,
                      button_window->height - 6, border_color);
    Context_draw_rect(button_window->context, 4, 4, button_window->width - 8,
                      button_window->height - 8, border_color);                                        
}

//This just sets and resets the toggle
void Button_mousedown_handler(Window* button_window, int x, int y) {

    Button* button = (Button*)button_window;

    button->color_toggle = !button->color_toggle;
}
