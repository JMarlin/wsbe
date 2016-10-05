#include <inttypes.h>
#include <stdlib.h>
#include "desktop.h"
#include "rect.h"


//================| Desktop Class Implementation |================//

Desktop* Desktop_new(Context* context) {

    //Malloc or fail 
    Desktop* desktop;
    if(!(desktop = (Desktop*)malloc(sizeof(Desktop))))
        return desktop;

    //Initialize the Window bits of our desktop
    if(!Window_init((Window*)desktop, 0, 0, context->width, context->height, WIN_NODECORATION, context)) {

        free(desktop);
        return (Desktop*)0;
    }

    //Override our paint function
    desktop->window.paint_function = Desktop_paint_handler;

    //Now continue by filling out the desktop-unique properties 
    desktop->window.last_button_state = 0;

    //Init mouse to the center of the screen
    desktop->mouse_x = desktop->window.context->width / 2;
    desktop->mouse_y = desktop->window.context->height / 2;

    return desktop;
}

//Paint the desktop 
void Desktop_paint_handler(Window* desktop_window) {
  
    //Fill the desktop
    Context_fill_rect(desktop_window->context, 0, 0, desktop_window->context->width, desktop_window->context->height, 0xFFFF9933);
}

//Our overload of the Window_process_mouse function used to capture the screen mouse position 
void Desktop_process_mouse(Desktop* desktop, uint16_t mouse_x,
                           uint16_t mouse_y, uint8_t mouse_buttons) {

    //Capture the mouse location in order to draw it later
    desktop->mouse_x = mouse_x;
    desktop->mouse_y = mouse_y;

    //Do the old generic mouse handling
    Window_process_mouse((Window*)desktop, mouse_x, mouse_y, mouse_buttons);

    //Window painting now happens inside of the window raise and move operations

    //And finally draw the hacky mouse, as usual
    Context_fill_rect(desktop->window.context, desktop->mouse_x, desktop->mouse_y, 10, 10, 0xFF000000);
}
