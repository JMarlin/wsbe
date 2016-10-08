#include <inttypes.h>
#include <stdlib.h>
#include "desktop.h"
#include "rect.h"


//================| Desktop Class Implementation |================//

//Mouse image data
#define CA 0xFF000000 //Black
#define CB 0xFFFFFFFF //White
#define CD 0x00000000 //Clear

unsigned int mouse_img[MOUSE_BUFSZ] = {
    CA, CD, CD, CD, CD, CD, CD, CD, CD, CD, CD,
    CA, CA, CD, CD, CD, CD, CD, CD, CD, CD, CD,
    CA, CB, CA, CD, CD, CD, CD, CD, CD, CD, CD,
    CA, CB, CB, CA, CD, CD, CD, CD, CD, CD, CD,
    CA, CB, CB, CB, CA, CD, CD ,CD, CD, CD, CD,
    CA, CB, CB, CB, CB, CA, CD, CD, CD, CD, CD,
    CA, CB, CB, CB, CB, CB, CA, CD, CD, CD, CD,
    CA, CB, CB, CB, CB, CB, CB, CA, CD, CD, CD,
    CA, CB, CB, CB, CB, CB, CB, CB, CA, CD, CD,
    CA, CB, CB, CB, CB, CB, CB, CB, CB, CA, CD,
    CA, CB, CB, CB, CB, CB, CB, CB, CB, CB, CA,
    CA, CA, CA, CA, CB, CB, CB, CA, CA, CA, CA,
    CD, CD, CD, CD, CA, CB, CB, CA, CD, CD, CD,
    CD, CD, CD, CD, CA, CB, CB, CA, CD, CD, CD,
    CD, CD, CD, CD, CD, CA, CB, CB, CA, CD, CD,
    CD, CD, CD, CD, CD, CA, CB, CB, CA, CD, CD,
    CD, CD, CD, CD, CD, CD, CA, CB, CA, CD, CD,
    CD, CD, CD, CD, CD, CD, CD, CA, CA, CD, CD 
};

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

    int i, x, y;
    Window* child;
    List* dirty_list;
    Rect* mouse_rect;

    //Do the old generic mouse handling
    Window_process_mouse((Window*)desktop, mouse_x, mouse_y, mouse_buttons);

    //Window painting now happens inside of the window raise and move operations

    //Build a dirty rect list for the mouse area
    if(!(dirty_list = List_new()))
        return;

    if(!(mouse_rect = Rect_new(desktop->mouse_y, desktop->mouse_x, 
                               desktop->mouse_y + MOUSE_HEIGHT - 1,
                               desktop->mouse_x + MOUSE_WIDTH - 1))) {

        free(dirty_list);
        return;
    }

    List_add(dirty_list, mouse_rect);

    //Do a dirty update for the desktop, which will, in turn, do a 
    //dirty update for all affected child windows
    Window_paint((Window*)desktop, dirty_list, 1); 

    //Clean up mouse dirty list
    List_remove_at(dirty_list, 0);
    free(dirty_list);
    free(mouse_rect);

    //Update mouse position
    desktop->mouse_x = mouse_x;
    desktop->mouse_y = mouse_y;

    //No more hacky mouse, instead we're going to rather inefficiently 
    //copy the pixels from our mouse image into the framebuffer
    for(y = 0; y < MOUSE_HEIGHT; y++) {

        //Make sure we don't draw off the bottom of the screen
        if((y + mouse_y) >= desktop->window.context->height)
            break;

        for(x = 0; x < MOUSE_WIDTH; x++) {

            //Make sure we don't draw off the right side of the screen
            if((x + mouse_x) >= desktop->window.context->width)
                break;
 
            //Don't place a pixel if it's transparent (still going off of ABGR here,
            //change to suit your palette)
            if(mouse_img[y * MOUSE_WIDTH + x] & 0xFF000000)
                desktop->window.context->buffer[(y + mouse_y)
                                                * desktop->window.context->width 
                                                + (x + mouse_x)
                                               ] = mouse_img[y * MOUSE_WIDTH + x];
        }
    }
}
