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
                //window's 31 px tall titlebar
                if(mouse_x >= child->x && mouse_x < (child->x + child->width) &&
                mouse_y >= child->y && mouse_y < (child->y + 31)) {

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
    unsigned int i, j;
    Window *current_window, *clipping_window;
    Rect* temp_rect;     
    List* clip_windows;                              
    
    //Do the clipping for the desktop just like before
    //Add a rect for the desktop
    temp_rect = Rect_new(0, 0, desktop->context->height - 1, desktop->context->width - 1);
    Context_add_clip_rect(desktop->context, temp_rect);

    //Now subtract each of the window rects from the desktop rect
    for(i = 0; i < desktop->children->count; i++) {
    
        current_window = (Window*)List_get_at(desktop->children, i);

        temp_rect = Rect_new(current_window->y, current_window->x,
                             current_window->y + current_window->height - 1,
                             current_window->x + current_window->width - 1);
        Context_subtract_clip_rect(desktop->context, temp_rect);
        free(temp_rect); //Rect doesn't end up in the clipping list
                         //during a subtract, so we need to get rid of it
    }

    //Fill the desktop
    Context_fill_rect(desktop->context, 0, 0, desktop->context->width, desktop->context->height, 0xFFFF9933);

    //Reset the context clipping 
    Context_clear_clip_rects(desktop->context);

    //Now we do a similar process to draw each window
    for(i = 0; i < desktop->children->count; i++) {
    
        current_window = (Window*)List_get_at(desktop->children, i);

        //Create and add a base rectangle for the current window
        temp_rect = Rect_new(current_window->y, current_window->x,
                             current_window->y + current_window->height - 1,
                             current_window->x + current_window->width - 1);
        Context_add_clip_rect(desktop->context, temp_rect);

        //Now, we need to get and clip any windows overlapping this one
        clip_windows = Desktop_get_windows_above(desktop, current_window);

        while(clip_windows->count) {
        
           //We do the different loop above and use List_remove_at because
           //we want to empty and destroy the list of clipping widows
           clipping_window = (Window*)List_remove_at(clip_windows, 0);

           //Make sure we don't try and clip the window from itself
           if(clipping_window == current_window)
               continue;

           //Get a rectangle from the window, subtract it from the clipping 
           //region, and dispose of it
           temp_rect = Rect_new(clipping_window->y, clipping_window->x,
                                clipping_window->y + clipping_window->height - 1,
                                clipping_window->x + clipping_window->width - 1);
           Context_subtract_clip_rect(desktop->context, temp_rect);
           free(temp_rect);
        }

        //Now that we've set up the clipping, we can do the
        //normal (but now clipped) window painting
        Window_paint(current_window);

        //Dispose of the used-up list and clear the clipping we used to draw the window
        free(clip_windows);
        Context_clear_clip_rects(desktop->context);
    }

    //Simple rectangle for the mouse
    Context_fill_rect(desktop->context, desktop->mouse_x, desktop->mouse_y, 10, 10, 0xFF000000);
}

//Used to get a list of windows overlapping the passed window
List* Desktop_get_windows_above(Desktop* desktop, Window* window) {

    int i;
    Window* current_window;
    List* return_list;

    //Attempt to allocate the output list
    if(!(return_list = List_new()))
        return return_list;

    //We just need to get a list of all items in the
    //child list at higher indexes than the passed window
    //We start by finding the passed child in the list
    for(i = 0; i < desktop->children->count; i++)
        if(window == (Window*)List_get_at(desktop->children, i))
            break;

    //Now we just need to add the remaining items in the list
    //to the output (IF they overlap, of course)
    //NOTE: As a bonus, this will also automatically fall through
    //if the window wasn't found
    for(; i < desktop->children->count; i++) {

        current_window = List_get_at(desktop->children, i);

        //Our good old rectangle intersection logic
        if(current_window->x <= (window->x + window->width - 1) &&
		   (current_window->x + current_window->width - 1) >= window->x &&
		   current_window->y <= (window->y + window->height - 1) &&
		   (window->y + window->height - 1) >= window->y)
            List_add(return_list, current_window); //Insert the overlapping window
    }

    return return_list; 
}