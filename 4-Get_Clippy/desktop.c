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

    //Create child list or clean up and fail
    if(!(desktop->children = List_new())) {

        //Delete the new desktop object and return null 
        free(desktop);
        return (Desktop*)0;
    }

    //Fill out other properties 
    desktop->context = context;
    desktop->last_button_state = 0;
    desktop->drag_child = (Window*)0;
    desktop->drag_off_x = 0;
    desktop->drag_off_y = 0;

    //Init mouse to the center of the screen
    desktop->mouse_x = desktop->context->width / 2;
    desktop->mouse_y = desktop->context->height / 2;

    return desktop;
}

//A method to automatically create a new window in the provided desktop 
Window* Desktop_create_window(Desktop* desktop, unsigned int x, unsigned int y,  
                          unsigned int width, unsigned int height) {

    //Attempt to create the window instance
    Window* window;
    if(!(window = Window_new(x, y, width, height, desktop->context)))
        return window;

    //Attempt to add the window to the end of the desktop's children list
    //If we fail, make sure to clean up all of our allocations so far 
    if(!List_add(desktop->children, (void*)window)) {

        free(window);
        return (Window*)0;
    }

    return window;
}

//Interface between windowing system and mouse device
void Desktop_process_mouse(Desktop* desktop, uint16_t mouse_x,
                           uint16_t mouse_y, uint8_t mouse_buttons) {

    int i;
    Window* child;

    desktop->mouse_x = mouse_x;
    desktop->mouse_y = mouse_y;

    //Check to see if mouse button has been depressed since last mouse update
    if(mouse_buttons) {
        
        //Events for mouse up -> down transition
        if(!desktop->last_button_state) {

            //If we had a button depressed, then we need to see if the mouse was
            //over any of the child windows
            //We go front-to-back in terms of the window stack for free occlusion
            for(i = desktop->children->count - 1; i >= 0; i--) {

                child = (Window*)List_get_at(desktop->children, i);

                //See if the mouse position lies within the bounds of the current
                //window 
                if(mouse_x >= child->x && mouse_x < (child->x + child->width) &&
                mouse_y >= child->y && mouse_y < (child->y + child->height)) {

                    //The mouse was over this window when the mouse was pressed, so
                    //we need to raise it
                    List_remove_at(desktop->children, i); //Pull window out of list
                    List_add(desktop->children, (void*)child); //Insert at the top 

                    //We'll also set this window as the window being dragged
                    //until such a time as the mouse is released
                    desktop->drag_off_x = mouse_x - child->x;
                    desktop->drag_off_y = mouse_y - child->y;
                    desktop->drag_child = child;

                    //Since we hit a window, we can stop looking
                    break;
                }
            }
        } 
    } else {

        //If the mouse is not down, we need to make sure our drag status is cleared
        desktop->drag_child = (Window*)0;
    }

    //Update drag window to match the mouse if we have an active drag window
    if(desktop->drag_child) {

        desktop->drag_child->x = mouse_x - desktop->drag_off_x;
        desktop->drag_child->y = mouse_y - desktop->drag_off_y;
    }

    //Now that we've handled any changes the mouse may have caused, we need to
    //update the screen to reflect those changes 
    Desktop_paint(desktop);

    //Update the stored mouse button state to match the current state 
    desktop->last_button_state = mouse_buttons;
}

//Paint the desktop 
void Desktop_paint(Desktop* desktop) {
  
    //Loop through all of the children and call paint on each of them 
    unsigned int i;
    Window* current_window;
    Rect* temp_rect;

    //Start by clearing the desktop background
    Context_fill_rect(desktop->context, 0, 0, desktop->context->width,
                      desktop->context->height, 0xFF000000); //Change pixel format if needed 
                                                            //Currently: ABGR

    //Instead of painting the windows, for now we'll add their dimensions to the context
    //clip rects and then draw those rects to show how our splitting algorithm works
    //Clear the old rects 
    Context_clear_clip_rects(desktop->context);
    
    //Add a rect for each window
    for(i = 0; (current_window = (Window*)List_get_at(desktop->children, i)); i++) {
    
        temp_rect = Rect_new(current_window->y, current_window->x,
                             current_window->y + current_window->height - 1,
                             current_window->x + current_window->width - 1);
        Context_add_clip_rect(desktop->context, temp_rect);
    }

    //Draw the clipping rects
    for(i = 0; i < desktop->context->clip_rects->count; i++) {

        temp_rect = (Rect*)List_get_at(desktop->context->clip_rects, i);
        Context_draw_rect(desktop->context, temp_rect->left, temp_rect->top,
                          temp_rect->right - temp_rect->left + 1,
                          temp_rect->bottom - temp_rect->top + 1,
                          0xFF00FF00);
    }

    //For now, we'll just draw a simple rectangle for the mouse (since that's
    //our only drawing primitive)
    Context_fill_rect(desktop->context, desktop->mouse_x, desktop->mouse_y, 10, 10, 0xFFFF0000);
}
